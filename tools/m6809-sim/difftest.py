#!/usr/bin/env python3
"""Differential test of m6809-sim against an independent Python 6809 core.

Each case is a random program: a preamble that loads every register with a
random value, then random instructions (any defined opcode, random operand
bytes, valid indexed postbytes) over random memory. Both cores run the same
number of instructions and must agree on every register, all of memory and
the cycle count. A case where the Python core faults (undefined opcode or
postbyte) must also stop the C simulator, as "undefined instruction".

usage: difftest.py PYTHON_CORE_DIR [CASES] [SEED]

PYTHON_CORE_DIR holds emu/mc6809.py (solartron-emu/scripts).
"""
import os
import random
import subprocess
import sys
import tempfile
import zlib

SIM = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'm6809-sim')
CODE = 0x1000
STEPS = 60


def fnv1a(data):
    h = 2166136261
    for b in data:
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return h


def main():
    sys.path.insert(0, sys.argv[1])
    from emu.mc6809 import MC6809, Bus, CPUFault

    cases = int(sys.argv[2]) if len(sys.argv) > 2 else 2000
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    rng = random.Random(seed)

    class FlatBus(Bus):
        def __init__(self, mem):
            self.mem = mem

        def read(self, a):
            return self.mem[a]

        def write(self, a, v):
            self.mem[a] = v

    # Opcodes the Python core defines, so random programs mostly run.
    probe = MC6809(FlatBus(bytearray(0x10000)))
    page0 = [op for op in range(256) if probe.page0[op] is not None
             and op not in (0x10, 0x11)]
    page2 = sorted(probe.page2)
    page3 = sorted(probe.page3)
    # Stops the C simulator handles differently on purpose.
    skip = {0x13, 0x3C}  # SYNC, CWAI: no interrupt source to wake them
    page0 = [op for op in page0 if op not in skip]

    def postbyte():
        if rng.random() < 0.3:
            return rng.randrange(0x80)
        mode = rng.choice([0, 1, 2, 3, 4, 5, 6, 8, 9, 0xB, 0xC, 0xD, 0xF])
        reg = rng.randrange(4) << 5
        if mode == 0xF:
            return 0x9F
        ind = 0x10 if mode not in (0, 2) and rng.random() < 0.3 else 0
        return 0x80 | reg | ind | mode

    def instruction():
        r = rng.random()
        if r < 0.08:
            out = [0x10, rng.choice(page2)]
        elif r < 0.11:
            out = [0x11, rng.choice(page3)]
        else:
            out = [rng.choice(page0)]
        op = out[-1]
        if op in (0x1E, 0x1F):  # TFR/EXG: same-size pairs
            regs16, regs8 = [0, 1, 2, 3, 4, 5], [8, 9, 10, 11]
            group = regs16 if rng.random() < 0.6 else regs8
            out.append(rng.choice(group) << 4 | rng.choice(group))
        elif (op & 0xF0) in (0x60, 0xA0, 0xE0) or op in range(0x30, 0x34):
            out.append(postbyte())
        out += [rng.randrange(256) for _ in range(3)]
        return out

    def preamble():
        p = []
        for op in (0x86, 0xC6):                 # LDA, LDB
            p += [op, rng.randrange(256)]
        for op in ([0x8E], [0x10, 0x8E], [0xCE], [0x10, 0xCE]):  # X Y U S
            v = rng.randrange(0x10000)
            p += op + [v >> 8, v & 0xFF]
        p += [0x1F, 0x8B]                       # TFR A,DP
        p += [0x86, rng.randrange(256)]         # LDA
        p += [0x1A, rng.randrange(256)]         # ORCC: random flags
        return p

    failures = 0
    full = 0
    compared = 0
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, 'mem.bin')
        for case in range(cases):
            mem = bytearray(rng.randrange(256) for _ in range(0x10000))
            prog = preamble()
            pre_len = len(prog)
            n_pre = 9
            if os.environ.get('ONE'):
                # One random instruction, then a parking loop (BRA *).
                prog += instruction() + [0x20, 0xFE]
            else:
                while len(prog) < 200:
                    prog += instruction()
            mem[CODE:CODE + len(prog)] = bytes(prog)
            mem[0xFFFE:0x10000] = CODE.to_bytes(2, 'big')

            py = MC6809(FlatBus(bytearray(mem)))
            py.cc = 0x50
            fault = None
            steps = 0
            for _ in range(n_pre + (1 if os.environ.get('ONE') else STEPS)):
                try:
                    py.step()
                    steps += 1
                except CPUFault as e:
                    fault = str(e)
                    break

            with open(path, 'wb') as f:
                f.write(mem)
            res = subprocess.run(
                [SIM, '-noports', '-dump', '-stats', '-load', '0',
                 '-entry', '%X' % CODE, '-max',
                 str(steps + 1 if fault else steps), path],
                capture_output=True, text=True)
            compared += steps
            if 'SYNC' in res.stderr or 'CWAI' in res.stderr:
                continue  # random jump into a wait instruction
            if fault:
                if res.returncode != 132:
                    print(f'case {case}: Python faulted ({fault}) but '
                          f'm6809-sim exited {res.returncode}')
                    failures += 1
                continue
            want = ('A=%02X B=%02X DP=%02X CC=%02X X=%04X Y=%04X U=%04X '
                    'S=%04X PC=%04X MEM=%08X' %
                    (py.a, py.b, py.dp, py.cc, py.x, py.y, py.u, py.s, py.pc,
                     fnv1a(py.bus.mem)))
            full += 1
            got = res.stdout.strip()
            cycles = res.stderr.split()[-2] if 'cycles' in res.stderr else '?'
            if res.returncode != 124 or got != want or \
                    cycles != str(py.cycles):
                print(f'case {case}: mismatch after {steps} instructions; '
                      f'first: {bytes(prog[pre_len:pre_len + 5]).hex()}')
                print(f'  python: {want} cycles={py.cycles}')
                print(f'  sim:    {got} cycles={cycles} '
                      f'exit={res.returncode} {res.stderr.strip()}')
                failures += 1
                if failures > 10:
                    break
    print(f'{cases} cases ({full} ran all {n_pre + STEPS} instructions, '
          f'{compared} instructions compared), {failures} failures')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
