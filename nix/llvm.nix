# clang, lld, llc, llvm-mc etc. with only the M6809 and M6800 targets, from
# the llvm-project fork. Built with clang: with GCC, LLVM turns off
# precompiled headers, which the backend relies on for a few missing
# #includes.
{ clangStdenv, cmake, ninja, python3, llvm-project }:

clangStdenv.mkDerivation {
  pname = "llvm-m6809";
  version = "unstable-${llvm-project.lastModifiedDate}";
  src = llvm-project;
  sourceRoot = "source/llvm";

  nativeBuildInputs = [ cmake ninja python3 ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DLLVM_ENABLE_ASSERTIONS=ON"
    "-DLLVM_TARGETS_TO_BUILD="
    "-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=M6809;M6800"
    "-DLLVM_ENABLE_PROJECTS=clang;lld"
    "-DCLANG_ENABLE_CIR=OFF"
    "-DLLVM_INCLUDE_BENCHMARKS=OFF"
    "-DLLVM_INCLUDE_EXAMPLES=OFF"
    "-DLLVM_INCLUDE_TESTS=OFF"
    "-DCLANG_INCLUDE_TESTS=OFF"
    "-DLLVM_PARALLEL_LINK_JOBS=2"
  ];

  meta.description = "LLVM, clang and lld with the experimental M6809 and M6800 backends";
}
