#!/usr/bin/env python3
"""Compare cycle counts: 6809 code on a 6809 against -mcpu=6309 code on an
HD6309 (both in the simulator, the HD6309 in emulation mode).

usage: run.py [--opt LEVEL] [WORKLOAD ...]

Each workload in workloads.c is built for both CPUs and run. The two runs
must return the same checksum. Counts include the shared setup (fill()).
"""
import argparse
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
CLANG = os.path.join(ROOT, 'build/bin/clang')
SIM = os.path.join(ROOT, 'tools/m6809-sim/m6809-sim')
WORKLOADS = ['checksum', 'bits', 'string', 'long', 'sort', 'struct',
             'convert']


def run(workload, cpu, opt, tmp):
    elf = os.path.join(tmp, f'{workload}-{cpu}.elf')
    subprocess.run([CLANG, '--target=m6809', f'-mcpu={cpu}', f'-O{opt}',
                    '-fno-builtin', '-w', f'-DWORKLOAD=w_{workload}',
                    os.path.join(HERE, 'workloads.c'), '-o', elf,
                    '-T', os.path.join(ROOT, 'runtime/sim/m6809-sim.ld'),
                    '-L', os.path.join(ROOT, 'runtime/build'), '-lm6809sim'],
                   check=True)
    sim = [SIM, '-stats'] + (['-6309'] if cpu == '6309' else []) + [elf]
    r = subprocess.run(sim, capture_output=True, text=True)
    m = re.search(r'(\d+) cycles', r.stderr)
    if not m:
        sys.exit(f'{workload} on {cpu}: no cycle count\n{r.stderr}')
    return r.returncode, int(m.group(1))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--opt', default='2')
    ap.add_argument('workloads', nargs='*', default=WORKLOADS)
    args = ap.parse_args()
    total = [0, 0]
    print(f'{"workload":12} {"6809":>9} {"6309":>9} {"ratio":>6}')
    with tempfile.TemporaryDirectory() as tmp:
        for w in args.workloads:
            r1, c1 = run(w, '6809', args.opt, tmp)
            r2, c2 = run(w, '6309', args.opt, tmp)
            if r1 != r2:
                sys.exit(f'{w}: checksums differ ({r1} on 6809, {r2} on 6309)')
            total[0] += c1
            total[1] += c2
            print(f'{w:12} {c1:9} {c2:9} {c2 / c1:6.2f}')
    print(f'{"total":12} {total[0]:9} {total[1]:9} {total[1] / total[0]:6.2f}')


if __name__ == '__main__':
    main()
