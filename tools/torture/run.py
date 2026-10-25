#!/usr/bin/env python3
"""Run GCC's c-torture/execute tests on m6809-sim.

usage: run.py [-O LEVEL] [-j JOBS] [--cpu CPU] [--sim-cpu CPU] [--max N] [TEST ...]

Each test is compiled with clang --target=m6809, linked for the simulator
and run. A test passes when it exits with status 0. Tests that cannot apply
to a 16-bit target without a hosted libc are skipped, by their dg- directives
(int32plus, trampolines, ...) or because they do not fit in 64K.

Tests listed in known-failures.txt fail for a reason other than a compiler
bug (most assume a 32-bit int); they are reported as XFAIL, or XPASS if they
pass after all.

Results go to results-O<LEVEL>-<CPU>.txt next to this script (one line per test)
and a summary is printed. TEST names a test (without .c) to run only those,
and prints each result.
"""
import argparse
import multiprocessing
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
TORTURE = os.path.join(ROOT, 'third_party/gcc/gcc/testsuite/gcc.c-torture/execute')
CLANG = os.path.join(ROOT, 'build/bin/clang')
SIM = os.path.join(ROOT, 'tools/m6809-sim/m6809-sim')

# Effective targets this toolchain does not have.
MISSING_TARGETS = {
    'int32plus', 'int32', 'int128', 'ptr32plus', 'size32plus', 'size20plus',
    'trampolines', 'untyped_assembly', 'dfp', 'dfprt', 'fileio', 'mmap',
    'c99_runtime', 'pthread', 'sync_int_long', 'sync_char_short', 'unwrapped',
    'run_expensive_tests', 'lto', 'alloca',
}

# Skips meant for other small targets that apply here too.
SKIP_PATTERNS = [
    r'dg-skip-if [^}]*\{\s*freestanding\s*\}',
    r'dg-skip-if "memory full',
    r'dg-skip-if "requires io"',
]

# Options from dg-options/dg-additional-options that clang understands and
# that change what the test means.
KEEP_OPTION = re.compile(r'^(-D\S+|-U\S+|-std=\S+|-fwrapv|-fno-strict-aliasing|'
                         r'-f(un)?signed-char|-fno-builtin\S*|-fno-inline|'
                         r'-ffast-math|-fno-math-errno|-finstrument-functions|'
                         r'-fgnu89-inline|-fno-common|-fno-strict-overflow|'
                         r'-fno-trapping-math)$')

BASE_FLAGS = ['--target=m6809', '-w', '-fno-color-diagnostics',
              '-Wno-error=implicit-function-declaration',
              '-Wno-error=implicit-int', '-Wno-error=int-conversion',
              '-Wno-error=incompatible-pointer-types',
              '-Wno-error=return-mismatch',
              '-Wno-error=incompatible-function-pointer-types',
              '-Wno-error=parentheses']
LINK_FLAGS = ['-T', 'm6809-sim.ld', '-lm6809sim']

# The stack size for tests that size their data by it (dg-add-options
# stack_size), and the most a test may ask for (dg-require-stack-size): all
# of the simulator's RAM, 0x100 to 0xFEFF.
STACK_SIZE = 8192
MAX_STACK = 0xFE00


def known_failures():
    """{test: reason} from known-failures.txt"""
    known = {}
    with open(os.path.join(HERE, 'known-failures.txt')) as f:
        for line in f:
            line = line.split('#', 1)[0].strip()
            if line:
                name, _, reason = line.partition(' ')
                known[name] = reason.strip()
    return known


def directives(src):
    """(skip reason or None, extra options)"""
    for m in re.finditer(r'dg-require-effective-target\s+(\w+)|'
                         r'dg-do run \{ target (\w+) \}', src):
        target = m.group(1) or m.group(2)
        if target in MISSING_TARGETS:
            return f'needs {target}', []
    m = re.search(r'dg-require-stack-size "([0-9x*+ ]+)"', src)
    if m and eval(m.group(1)) > MAX_STACK:
        return f'needs {eval(m.group(1))} bytes of stack', []
    for pat in SKIP_PATTERNS:
        if re.search(pat, src):
            return 'skipped for small targets', []
    opts = []
    for m in re.finditer(r'dg-(?:additional-)?options\s+(?:\{\s*)?"([^"]*)"', src):
        opts += [o for o in m.group(1).split() if KEEP_OPTION.match(o)]
    if re.search(r'dg-add-options\s+stack_size', src):
        opts.append(f'-DSTACK_SIZE={STACK_SIZE}')
    return None, opts


def run_one(args):
    name, opt, cpu, sim_cpu, max_insns = args
    path = os.path.join(TORTURE, name + '.c')
    with open(path, errors='replace') as f:
        src = f.read()
    skip, extra = directives(src)
    if skip:
        return name, 'SKIP', skip
    with tempfile.TemporaryDirectory() as tmp:
        exe = os.path.join(tmp, 'a.elf')
        cmd = [CLANG] + BASE_FLAGS + [f'-O{opt}', f'-mcpu={cpu}'] + extra + \
            [path, '-o', exe] + LINK_FLAGS
        try:
            r = subprocess.run(cmd, capture_output=True, text=True,
                               timeout=300, cwd=tmp)
            # GCC defaults to gnu23 and clang to gnu17. Clang's C23 rejects
            # the implicit int of old tests, so C23 is only the fallback.
            if r.returncode and 'error:' in r.stderr and \
                    not any(o.startswith('-std=') for o in extra):
                r23 = subprocess.run(cmd + ['-std=gnu23'], capture_output=True,
                                     text=True, timeout=300, cwd=tmp)
                if not r23.returncode:
                    r = r23
        except subprocess.TimeoutExpired:
            return name, 'COMPILE-TIMEOUT', ''
        if r.returncode:
            err = r.stderr
            if 'will not fit in region' in err or 'section overflow' in err or \
                    'is out of range' in err and 'RAM' in err or \
                    'array is too large' in err:
                return name, 'SKIP', 'does not fit in 64K'
            if 'PLEASE submit a bug report' in err or 'error in backend' in err:
                first = next((l for l in err.splitlines()
                              if 'error' in l.lower() or 'Assertion' in l),
                             err.splitlines()[0] if err else '')
                return name, 'ICE', first.strip()[:200]
            if 'undefined symbol' in err:
                syms = sorted(set(re.findall(r'undefined symbol: (\S+)', err)))
                return name, 'LINK-ERROR', ' '.join(syms)[:200]
            first = next((l for l in err.splitlines() if 'error' in l), '')
            return name, 'COMPILE-ERROR', first.strip()[:200]
        try:
            sim_flags = ['-6309'] if sim_cpu == '6309' else []
            r = subprocess.run([SIM, '-max', str(max_insns)] + sim_flags +
                               [exe],
                               capture_output=True, text=True,
                               errors='replace', timeout=600)
        except subprocess.TimeoutExpired:
            return name, 'TIMEOUT', 'wall clock'
        if r.returncode == 0:
            return name, 'PASS', ''
        if r.returncode == 124:
            return name, 'TIMEOUT', 'instruction limit'
        detail = {134: 'abort', 132: 'undefined instruction',
              133: 'division by zero'}.get(
            r.returncode, f'exit {r.returncode}')
        return name, 'FAIL', (detail + ' ' + r.stderr.strip())[:200]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-O', dest='opt', default='2')
    ap.add_argument('-j', dest='jobs', type=int,
                    default=multiprocessing.cpu_count())
    ap.add_argument('--cpu', default='6809')
    ap.add_argument('--sim-cpu', help='CPU to simulate (default: --cpu); '
                    '--cpu 6809 --sim-cpu 6309 checks that 6809 code runs '
                    'unchanged on an HD6309')
    ap.add_argument('--max', type=int, default=3_000_000_000)
    ap.add_argument('tests', nargs='*')
    a = ap.parse_args()

    names = a.tests or sorted(f[:-2] for f in os.listdir(TORTURE)
                              if f.endswith('.c'))
    sim_cpu = a.sim_cpu or a.cpu
    work = [(n, a.opt, a.cpu, sim_cpu, a.max) for n in names]
    known = known_failures()
    results = []
    with multiprocessing.Pool(a.jobs) as pool:
        for i, res in enumerate(pool.imap_unordered(run_one, work), 1):
            name, status, detail = res
            if name in known and status != 'SKIP':
                res = (name, 'XPASS' if status == 'PASS' else 'XFAIL',
                       known[name] if status != 'PASS' else '')
            results.append(res)
            if a.tests:
                print(f'{res[0]}: {res[1]} {res[2]}')
            elif i % 100 == 0:
                print(f'{i}/{len(work)}', file=sys.stderr)
    results.sort()
    counts = {}
    for _, status, _ in results:
        counts[status] = counts.get(status, 0) + 1
    if not a.tests:
        out = os.path.join(HERE, f'results-O{a.opt}-{a.cpu}' +
                           (f'-on-{sim_cpu}' if sim_cpu != a.cpu else '') +
                           '.txt')
        with open(out, 'w') as f:
            for name, status, detail in results:
                f.write(f'{name}\t{status}\t{detail}\n')
        print(f'wrote {out}')
    applicable = len(results) - counts.get('SKIP', 0) - counts.get('XFAIL', 0)
    print(' '.join(f'{k}={v}' for k, v in sorted(counts.items())))
    passed = counts.get('PASS', 0) + counts.get('XPASS', 0)
    if applicable:
        print(f'{passed}/{applicable} applicable tests pass '
              f'({100 * passed / applicable:.1f}%)')


if __name__ == '__main__':
    main()
