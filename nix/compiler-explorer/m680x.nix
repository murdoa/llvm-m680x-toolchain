# Compiler Explorer with only the m6809 and m6800 compilers configured.
# Extra arguments go to compiler-explorer, e.g. --port 8080.
{ lib, writeShellScript, writeShellApplication, runCommand, coreutils, findutils, git
, compiler-explorer, toolchain, m6809-sim, m6800-sim }:

let
  # Runs exe without the flags CE adds for host tools that ours reject: the
  # x86 syntax flag of its Intel-syntax filter (for objdump, llc and rustc)
  # and GNU c++filt's --no-verbose.
  ceTool = name: exe: writeShellScript name ''
    args=()
    for a in "$@"; do
      case $a in
        *x86-asm-syntax=*|--no-verbose) ;;
        *) args+=("$a") ;;
      esac
    done
    exec ${exe} "''${args[@]}"
  '';
  objdump = ceTool "m6809-objdump" "${toolchain}/bin/llvm-objdump";
  llc = ceTool "m6809-llc" "${toolchain}/bin/llc";
  cxxfilt = ceTool "m6809-cxxfilt" "${toolchain}/bin/llvm-cxxfilt";
  # TODO: build rustc in nix. See rustc.sh.
  rustc = writeShellScript "m6809-rustc"
    (builtins.replaceStrings [ "@rt@" ] [ "${./rust-rt}" ]
      (builtins.readFile ./rustc.sh));
  rootDir = runCommand "compiler-explorer-m6809-etc" { } ''
    cp -r ${compiler-explorer}/share/compiler-explorer/etc $out
    chmod -R u+w $out
    for f in ${./.}/*.properties; do
      substitute $f $out/config/$(basename $f) \
        --replace-quiet @toolchain@ ${toolchain} \
        --replace-quiet @m6809sim@ ${m6809-sim} \
        --replace-quiet @m6800sim@ ${m6800-sim} \
        --replace-quiet @rustc@ ${rustc} \
        --replace-quiet @llc@ ${llc} \
        --replace-quiet @cxxfilt@ ${cxxfilt} \
        --replace-quiet @objdump@ ${objdump}
    done
  '';
in writeShellApplication {
  name = "compiler-explorer-m6809";
  runtimeInputs = [ coreutils findutils git ];
  text = builtins.replaceStrings [ "@ce@" "@etc@" ]
    [ (lib.getExe compiler-explorer) "${rootDir}" ]
    (builtins.readFile ./wrapper.sh);
}
