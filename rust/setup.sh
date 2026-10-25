#!/bin/sh
# Applies the m6809/m6800 patches to a rust checkout in third_party/rust at
# the commit they were made on, and writes its bootstrap.toml. Build the
# LLVM it links against first (see README.md).
set -e
TOP=$(cd "$(dirname "$0")/.." && pwd)
RUST=$TOP/third_party/rust
BASE=dc44e7f2c
CC=$HOME/.cargo/registry/src/index.crates.io-1949cf8c6b5b557f/cc-1.2.62

cd "$RUST"
git checkout -q -B m6809 $BASE
git am -q "$TOP"/rust/patches/000*.patch

# cc 1.2.62, as bootstrap pins it, with the two architectures added.
[ -d "$CC" ] || cargo fetch --manifest-path src/bootstrap/Cargo.toml
rm -rf src/bootstrap/cc-patched
cp -r "$CC" src/bootstrap/cc-patched
rm -f src/bootstrap/cc-patched/.cargo-ok src/bootstrap/cc-patched/.cargo_vcs_info.json
(cd src/bootstrap/cc-patched && patch -s -p1 < "$TOP"/rust/patches/cc-1.2.62-m6809.diff)

HOST=$(rustc -vV | sed -n 's/^host: //p')
cat > bootstrap.toml <<TOML
# rustc against the LLVM in build-rust, which has the M6809 and M6800
# backends.
change-id = "ignore"
profile = "compiler"

[llvm]
download-ci-llvm = false

[build]
build-stage = 1
docs = false
extended = false
tools = []

[rust]
download-rustc = false
codegen-backends = ["llvm"]
lld = false
llvm-tools = false
debug-assertions = false
incremental = true
# core has dead-code warnings on 16-bit targets without atomics, as on msp430.
deny-warnings = false

[target.$HOST]
llvm-config = "$TOP/build-rust/bin/llvm-config"
TOML
for t in m6809 m6800; do cat >> bootstrap.toml <<TOML

[target.$t-none-elf]
cc = "$TOP/build/bin/clang"
ar = "$TOP/build/bin/llvm-ar"
linker = "$TOP/build/bin/clang"
TOML
done
echo "now: cd $RUST && ./x build library --stage 1 --target m6809-none-elf,m6800-none-elf"
