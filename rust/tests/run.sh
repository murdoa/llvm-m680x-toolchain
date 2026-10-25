#!/bin/sh
# Builds tests/*.rs as no_std static libraries for the 6809, HD6309 and
# HD6301 at -O0, -O2 and -Oz, links them with clang and the simulator runtime, and runs them.
# Each test's main returns 0 on success.
set -u
TOP=$(cd "$(dirname "$0")/../.." && pwd)
RUSTC=${RUSTC:-$(ls -d "$TOP"/third_party/rust/build/*/stage1/bin/rustc | head -1)}
OUT=$TOP/rust/tests/out
mkdir -p "$OUT"
fail=0
for src in "$TOP"/rust/tests/*.rs; do
  name=$(basename "$src" .rs)
  # target:cpu, with the simulator flag for the CPU.
  for tc in m6809:6809 m6809:6309 m6800:6301; do
    t=${tc%:*} cpu=${tc#*:}
    simflags=; [ $cpu = 6309 ] && simflags=-6309
    for o in 0 2 z; do
      base=$OUT/$name-$cpu-O$o
      if "$RUSTC" --target $t-none-elf -C target-cpu=$cpu -C opt-level=$o \
           -C panic=abort --crate-type staticlib "$src" -o "$base.a" &&
         "$TOP"/build/bin/clang --target=$t -mcpu=$cpu "$base.a" \
           -Wl,--gc-sections -T $t-sim.ld -l${t}sim -o "$base.elf" &&
         "$TOP"/tools/$t-sim/$t-sim $simflags -max 100000000 "$base.elf"; then
        echo "PASS $name $cpu -O$o"
      else
        echo "FAIL $name $cpu -O$o (exit $?)"; fail=1
      fi
    done
  done
done
exit $fail
