#!/usr/bin/env bash
# Configure an LLVM build with the experimental M6809 target.
# Run inside `nix develop`. Re-run after adding new targets.
set -euo pipefail
cd "$(dirname "$0")"

cmake -S llvm-project/llvm -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_TARGETS_TO_BUILD="${HOST_TARGETS:-}" \
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="${TARGETS-M6809}" \
  -DLLVM_ENABLE_PROJECTS="${PROJECTS-clang;lld}" \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DLLVM_USE_LINKER=lld \
  -DLLVM_CCACHE_BUILD=ON \
  -DLLVM_OPTIMIZED_TABLEGEN=ON \
  -DLLVM_PARALLEL_LINK_JOBS=2 \
  -DLLVM_INCLUDE_BENCHMARKS=OFF \
  -DLLVM_INCLUDE_EXAMPLES=OFF
