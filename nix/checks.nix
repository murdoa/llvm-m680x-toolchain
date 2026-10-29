# The flake's checks: the runtime tests and the template, built with the
# toolchain and run on the simulators.
{ runCommand, gnumake, toolchain, m6809-sim, m6800-sim }:

let
  sims = { m6809 = m6809-sim; m6800 = m6800-sim; };

  # Each test in runtime/tests prints its result and exits 0 when it
  # passes (as `make -C runtime check`).
  runtime = name: target: cpuFlag: simFlag:
    runCommand "runtime-tests-${name}" { } ''
      fail=0
      for t in ${../runtime/tests}/*.c ${../runtime/tests}/*.cpp; do
        n=$(basename $t)
        case $n in
          *.cpp) cc="${toolchain}/bin/clang++ -std=c++26" ;;
          *) cc=${toolchain}/bin/clang ;;
        esac
        if ! $cc --target=${target} ${cpuFlag} -O2 $t -T ${target}-sim.ld \
             -l${target}sim -o $n.elf; then
          echo "$n: does not build"; fail=1; continue
        fi
        if ! ${sims.${target}}/bin/${target}-sim ${simFlag} -max 500000000 $n.elf; then
          echo "$n: fails"; fail=1
        fi
      done
      [ $fail = 0 ] && touch $out
    '';
in {
  runtime-m6809 = runtime "m6809" "m6809" "" "";
  runtime-hd6309 = runtime "hd6309" "m6809" "-mcpu=6309" "-6309";
  runtime-m6800 = runtime "m6800" "m6800" "" "";

  # templates/hello builds and runs for both targets.
  template = runCommand "template-hello" {
    nativeBuildInputs = [ gnumake toolchain m6809-sim m6800-sim ];
  } ''
    cp -r ${../templates/hello} hello
    chmod -R u+w hello
    cd hello
    make run TARGET=m6809 | tee m6809.out
    make run TARGET=m6800 | tee m6800.out
    grep -q 'Hello' m6809.out && grep -q 'Hello' m6800.out
    touch $out
  '';
}
