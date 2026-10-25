#!/usr/bin/env python3
"""Differential test of m6800-sim against an independent Python HD6301 core.

Each case is a random program: a preamble that loads every register with a
random value, then random instructions (any documented opcode, random
operand bytes) over random memory, itself mostly documented opcodes. Both cores run the same number of
instructions and must agree on every register, all of memory and the cycle
count. A case where the Python core faults (undefined opcode) must also stop
the C simulator, as "undefined instruction".

The Python core models the on-chip registers at $00-$1F and RAM at
$80-$FF; here its read and write are replaced by flat memory, so direct
mode sees the same plain memory as m6800-sim.

usage: difftest.py PYTHON_CORE_DIR [CASES] [SEED]

PYTHON_CORE_DIR holds emu/hd6301.py (solartron/scripts).
"""
import os
import random
import subprocess
import sys
import tempfile

SIM = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'm6800-sim')
CODE = 0x1000
STEPS = 60


def fnv1a(data):
    h = 2166136261
    for b in data:
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return h


def main():
    sys.path.insert(0, sys.argv[1])
    from emu.hd6301 import HD6301, Bus, CPUFault, CYCLES, UNDOCUMENTED

    cases = int(sys.argv[2]) if len(sys.argv) > 2 else 2000
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    rng = random.Random(seed)

    class FlatBus(Bus):
        def __init__(self, mem):
            self.mem = mem

    class FlatCPU(HD6301):
        # Plain memory everywhere: no on-chip registers or RAM.
        def read(self, address):
            return self.bus.mem[address]

        def write(self, address, value):
            self.bus.mem[address] = value & 0xFF

    # Opcodes the Python core defines, so random programs mostly run.
    ops = [op for op in range(256)
           if CYCLES[op] is not None and op not in UNDOCUMENTED]
    # Stops the C simulator handles differently on purpose.
    skip = {0x1A, 0x3E}  # SLP, WAI: no interrupt source to wake them
    ops = [op for op in ops if op not in skip]

    def instruction():
        return [rng.choice(ops)] + [rng.randrange(256) for _ in range(3)]

    def preamble():
        s = rng.randrange(0x10000)
        p = [0xC6, rng.randrange(256)]                 # LDAB
        x = rng.randrange(0x10000)
        p += [0xCE, x >> 8, x & 0xFF]                  # LDX
        p += [0x8E, (s - 1) >> 8 & 0xFF, (s - 1) & 0xFF]  # LDS
        p += [0x86, rng.randrange(256), 0x06]          # LDAA, TAP: random flags
        p += [0x32]                                    # PULA: random A
        return p

    failures = 0
    full = 0
    compared = 0
    n_pre = 6
    n_run = n_pre + (1 if os.environ.get('ONE') else STEPS)
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, 'mem.bin')
        for case in range(cases):
            # Mostly defined opcodes, so random jumps keep running.
            mem = bytearray(rng.choice(ops) if rng.random() < 0.8
                            else rng.randrange(256) for _ in range(0x10000))
            prog = preamble()
            pre_len = len(prog)
            if os.environ.get('ONE'):
                # One random instruction, then a parking loop (BRA *).
                prog += instruction() + [0x20, 0xFE]
            else:
                while len(prog) < 200:
                    prog += instruction()
            mem[CODE:CODE + len(prog)] = bytes(prog)

            py = FlatCPU(FlatBus(bytearray(mem)))
            py.pc = CODE
            fault = None
            steps = 0
            for _ in range(n_run):
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
            if 'WAI' in res.stderr or 'SLP' in res.stderr:
                continue  # random jump into a wait instruction
            if fault:
                if res.returncode != 132:
                    print(f'case {case}: Python faulted ({fault}) but '
                          f'm6800-sim exited {res.returncode}')
                    failures += 1
                continue
            want = ('A=%02X B=%02X CC=%02X X=%04X S=%04X PC=%04X MEM=%08X' %
                    (py.a, py.b, py.cc, py.x, py.s, py.pc,
                     fnv1a(py.bus.mem)))
            full += 1
            got = res.stdout.strip()
            cycles = res.stderr.split()[-2] if 'cycles' in res.stderr else '?'
            if res.returncode != 124 or got != want or \
                    cycles != str(py.cycles):
                print(f'case {case}: mismatch after {steps} instructions; '
                      f'first: {bytes(prog[pre_len:pre_len + 4]).hex()}')
                print(f'  python: {want} cycles={py.cycles}')
                print(f'  sim:    {got} cycles={cycles} '
                      f'exit={res.returncode} {res.stderr.strip()}')
                failures += 1
                if failures > 10:
                    break
    print(f'{cases} cases ({full} ran all {n_run} instructions, '
          f'{compared} instructions compared), {failures} failures')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
