#!/usr/bin/env python3
"""Compare code size per function: clang -Oz against CMOC.

usage: compare.py [--cpu CPU] [--opt LEVEL] [FILE.c ...]

Each file in bench/ (or each FILE given) is compiled with clang for m6809
and with CMOC, and the size of every function is printed with the total.
Only code is counted, not data, and not the runtime routines each compiler
calls (CMOC's are larger than ours, which would flatter clang).

CMOC and lwtools are looked for in third_party/prefix/bin (see README.md).
"""
import argparse
import glob
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
CLANG = os.path.join(ROOT, 'build/bin/clang')
NM = os.path.join(ROOT, 'build/bin/llvm-nm')
PREFIX = os.path.join(ROOT, 'third_party/prefix/bin')


def clang_sizes(path, cpu, opt):
    with tempfile.TemporaryDirectory() as tmp:
        obj = os.path.join(tmp, 'a.o')
        # -fno-builtin: do not turn the loops into calls to libc.
        subprocess.run([CLANG, '--target=m6809', f'-mcpu={cpu}', f'-O{opt}',
                        '-fno-builtin', '-c', path, '-o', obj], check=True)
        out = subprocess.run([NM, '-S', '--defined-only', obj],
                             capture_output=True, text=True, check=True).stdout
    sizes = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[2] in 'Tt':
            sizes[parts[3]] = int(parts[1], 16)
    return sizes


def cmoc_sizes(path):
    env = dict(os.environ, PATH=PREFIX + os.pathsep + os.environ['PATH'])
    with tempfile.TemporaryDirectory() as tmp:
        obj = os.path.join(tmp, 'a.o')
        r = subprocess.run([os.path.join(PREFIX, 'cmoc'), '-c', '-o', obj,
                            path], capture_output=True, text=True, env=env)
        if r.returncode:
            sys.exit(f'cmoc failed on {path}:\n{r.stderr}')
        out = subprocess.run([os.path.join(PREFIX, 'lwobjdump'), obj],
                             capture_output=True, text=True,
                             check=True).stdout
    return {m.group(1): int(m.group(2), 16)
            for m in re.finditer(r'funcsize_(\w+)=([0-9A-Fa-f]+)', out)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cpu', default='6809')
    ap.add_argument('--opt', default='z')
    ap.add_argument('files', nargs='*')
    a = ap.parse_args()
    files = a.files or sorted(glob.glob(os.path.join(HERE, 'bench', '*.c')))

    total_c = total_m = 0
    print(f'{"function":24} {"clang":>6} {"cmoc":>6}  ratio')
    for path in files:
        ours = clang_sizes(path, a.cpu, a.opt)
        theirs = cmoc_sizes(path)
        print(f'-- {os.path.basename(path)}')
        for name in sorted(theirs):
            c, m = ours.get(name), theirs[name]
            if c is None:  # inlined or removed by clang
                continue
            total_c += c
            total_m += m
            print(f'{name:24} {c:6} {m:6}  {c / m:5.2f}')
    print(f'{"total":24} {total_c:6} {total_m:6}  {total_c / total_m:5.2f}')


if __name__ == '__main__':
    main()
