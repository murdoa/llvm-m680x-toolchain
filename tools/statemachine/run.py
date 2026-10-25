#!/usr/bin/env python3
"""Differential test of machine.cpp: every target build must print what the
host build prints.

machine.cpp prints run_machine's result for every event sequence of length
0, 1 and 2 over all byte values, and checks the compile-time evaluate<>()
of each pair of enumerators. The host build (the system C++ compiler, or
build/bin/clang++ when there is none) is the reference. Each target build
runs on the simulator. Builds cover the 6809, HD6309 and HD6301 at -O0
to -Oz, with and without LTO.

For each build it prints the size of run_machine and main and the cycles
the whole run took.

usage: run.py [-v]
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
BIN = os.path.join(ROOT, 'build/bin')
SRC = os.path.join(HERE, 'machine.cpp')
CXXFLAGS = ['-x', 'c++', '-std=c++20', '-fno-exceptions', '-fno-rtti']

CPUS = [('m6809', '6809', []), ('m6809', '6309', ['-6309']),
        ('m6800', '6301', [])]
OPTS = ['-O0', '-O1', '-O2', '-O3', '-Os', '-Oz']


def host_reference(tmp):
    cxx = shutil.which('c++') or os.path.join(BIN, 'clang++')
    exe = os.path.join(tmp, 'host')
    subprocess.run([cxx, '-std=c++20', '-DHOST', '-O0', SRC, '-o', exe],
                   check=True)
    return subprocess.run([exe], check=True, capture_output=True).stdout


def symbol_sizes(elf):
    out = subprocess.run([os.path.join(BIN, 'llvm-nm'), '-S', elf],
                         check=True, capture_output=True, text=True).stdout
    sizes = {}
    for line in out.splitlines():
        f = line.split()
        if len(f) == 4 and f[3] in ('run_machine', 'main'):
            sizes[f[3]] = int(f[1], 16)
    return sizes


def target_run(tmp, target, cpu, simflags, opt, lto):
    elf = os.path.join(tmp, f'{cpu}{opt}{"-lto" if lto else ""}.elf')
    cmd = [os.path.join(BIN, 'clang'), f'--target={target}', f'-mcpu={cpu}',
           opt] + CXXFLAGS + (['-flto'] if lto else []) + [
           SRC, '-T', f'{target}-sim.ld', f'-l{target}sim',
           '-Wl,--gc-sections', '-o', elf]
    subprocess.run(cmd, check=True)
    sim = os.path.join(ROOT, f'tools/{target}-sim/{target}-sim')
    r = subprocess.run([sim, '-stats'] + simflags + [elf],
                       capture_output=True)
    m = re.search(r'(\d+) cycles', r.stderr.decode(errors='replace'))
    cycles = int(m.group(1)) if m else None
    return r.returncode, r.stdout, symbol_sizes(elf), cycles


def first_difference(want, got):
    wl, gl = want.split(b'\n'), got.split(b'\n')
    for i, (w, g) in enumerate(zip(wl, gl)):
        if w != g:
            j = next(k for k in range(min(len(w), len(g)) + 1)
                     if k == min(len(w), len(g)) or w[k] != g[k])
            return f'line {i}, column {j}'
    return f'{len(gl)} lines, want {len(wl)}'


def main():
    verbose = '-v' in sys.argv
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        want = host_reference(tmp)
        for target, cpu, simflags in CPUS:
            for opt in OPTS:
                for lto in (False, True):
                    name = f'{cpu} {opt}{" lto" if lto else ""}'
                    rc, got, sizes, cycles = target_run(
                        tmp, target, cpu, simflags, opt, lto)
                    ok = rc == 0 and got == want
                    failures += not ok
                    detail = (f'run_machine {sizes.get("run_machine", "-")} '
                              f'main {sizes.get("main", "-")} '
                              f'cycles {cycles}')
                    if not ok:
                        detail += (f'  exit {rc}' if rc else
                                   f'  differs at {first_difference(want, got)}')
                    if verbose or not ok:
                        print(f'{"PASS" if ok else "FAIL"}  {name:14} {detail}')
    print(f'{"PASS" if not failures else "FAIL"}: {failures} failures')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
