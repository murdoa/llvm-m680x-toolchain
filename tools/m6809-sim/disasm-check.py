#!/usr/bin/env python3
"""Check the LLVM 6809 disassembler against an independent Python 6809 core.

For every opcode on every page, and every indexed postbyte, both must agree
on whether the bytes are a valid instruction and on its length. The length
from the Python core is the number of bytes it fetches while executing the
instruction, so it does not depend on where the instruction jumps.

usage: disasm-check.py PYTHON_CORE_DIR LLVM_BIN_DIR

PYTHON_CORE_DIR holds emu/mc6809.py (solartron-emu/scripts).
"""
import os
import re
import subprocess
import sys
import tempfile

SLOT = 16
TAIL = [0xA5, 0x5A, 0x3C]


def candidates():
    for prefix in ([], [0x10], [0x11]):
        for op in range(256):
            if not prefix and op in (0x10, 0x11):
                continue
            for post in range(256):
                yield prefix + [op, post] + TAIL


def main():
    sys.path.insert(0, sys.argv[1])
    from emu.mc6809 import MC6809, Bus, CPUFault
    bindir = sys.argv[2]

    class FlatBus(Bus):
        def __init__(self, mem):
            self.mem = mem

        def read(self, a):
            return self.mem[a]

        def write(self, a, v):
            self.mem[a] = v

    def python_length(code):
        mem = bytearray(0x10000)
        mem[0x4000:0x4000 + len(code)] = bytes(code)
        mem[0xFFFE:0x10000] = b'\x40\x00'
        cpu = MC6809(FlatBus(mem))
        cpu.s = 0x8000
        cpu.u = 0x9000
        fetched = 0
        orig = cpu.fetch

        def counting_fetch():
            nonlocal fetched
            fetched += 1
            return orig()
        cpu.fetch = counting_fetch
        try:
            cpu.step()
        except CPUFault:
            return None
        return fetched

    cands = list(candidates())
    blob = bytearray()
    for c in cands:
        blob += bytes(c) + b'\x12' * (SLOT - len(c))  # NOP padding resyncs

    with tempfile.TemporaryDirectory() as tmp:
        raw = os.path.join(tmp, 'blob.bin')
        obj = os.path.join(tmp, 'blob.o')
        with open(raw, 'wb') as f:
            f.write(blob)
        subprocess.run([os.path.join(bindir, 'llvm-objcopy'), '-I', 'binary',
                        '-O', 'elf32-m6809', '--rename-section=.data=.text,code',
                        raw, obj], check=True)
        out = subprocess.run([os.path.join(bindir, 'llvm-objdump'), '-d',
                              '--triple=m6809', obj],
                             capture_output=True, text=True, check=True).stdout

    # address -> (length, text)
    llvm = {}
    for line in out.splitlines():
        m = re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(.*)', line)
        if m:
            addr = int(m.group(1), 16)
            llvm[addr] = (len(m.group(2).split()), m.group(3).strip())

    failures = 0
    valid = 0
    for i, c in enumerate(cands):
        got = llvm.get(i * SLOT)
        if got is None:
            print(f'{bytes(c).hex()}: no line at slot start (desync)')
            failures += 1
            continue
        length, text = got
        llvm_len = None if '<unknown>' in text else length
        py_len = python_length(c)
        if llvm_len != py_len:
            failures += 1
            if failures <= 20:
                print(f'{bytes(c).hex()}: llvm {llvm_len} ({text}), '
                      f'python {py_len}')
        elif py_len:
            valid += 1
    print(f'{len(cands)} byte sequences, {valid} valid, {failures} mismatches')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
