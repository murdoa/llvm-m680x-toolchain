# Code size against CMOC

`compare.py` compiles each file in `bench/` with clang (`-Oz` by default)
and with [CMOC](http://sarrazip.com/dev/cmoc.html), and prints the size of
each function. Only function code is counted.

CMOC needs lwtools. Both build from source into `third_party/prefix`:

    curl -LO https://www.lwtools.ca/releases/lwtools/lwtools-4.24.tar.gz
    curl -LO http://sarrazip.com/dev/cmoc-0.1.100.tar.gz
    # lwtools: make, then copy lwasm, lwlink, lwar, lwobjdump to prefix/bin
    # cmoc (needs bison and flex): ./configure --prefix=$PREFIX && make install

The benchmarks are small routines of the kind found in 8-bit firmware. They
are written so CMOC accepts them too (it rejects some implicit narrowing).

## Status

At `-Oz` the clang total is 2323 bytes against 2377 for CMOC 0.1.100
(ratio 0.98). The biggest gap left is `isqrt32` (32-bit locals that
CMOC keeps in memory).
