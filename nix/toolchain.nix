# The LLVM build plus the runtime for both targets (crt0, compiler-rt
# builtins, libc, libc++ and the simulator board libraries), so that
# `clang --target=m6809 x.c`, `clang++ -std=c++26` and `--target=m6800`
# link. clang finds the runtime relative to its own binary, so the tools are
# copied rather than symlinked.
{ stdenv, cmake, ninja, python3, llvm, llvm-project, pname ? "m6809-toolchain" }:

stdenv.mkDerivation {
  inherit pname;
  inherit (llvm) version;
  src = ../runtime;

  # cmake generates the libc++ headers; the runtime Makefile runs it.
  nativeBuildInputs = [ cmake ninja python3 ];
  dontConfigure = true;

  buildPhase = ''
    runHook preBuild
    mkdir -p $out/bin $out/lib
    for t in clang clang++ clang-cpp ld.lld lld llc llvm-mc llvm-ar \
             llvm-ranlib llvm-objdump llvm-objcopy llvm-nm llvm-size \
             llvm-readobj llvm-readelf llvm-cxxfilt; do
      cp -P ${llvm}/bin/$t $out/bin/
    done
    # clang and friends are symlinks to clang-NN; copy that too.
    cp ${llvm}/bin/clang-[0-9]* $out/bin/
    cp -r ${llvm}/lib/clang $out/lib/
    chmod -R u+w $out

    for target in m6809 m6800; do
      make install TARGET=$target LLVM_BUILD=$out \
        LLVM_SRC=${llvm-project} -j$NIX_BUILD_CORES
    done
    runHook postBuild
  '';

  dontInstall = true;
  # The host strip would mangle the 6809 and 6800 objects.
  dontStrip = true;

  meta.description = "clang toolchain and runtime for the M6809 and M6800";
}
