# The instruction-set simulators the runtime tests, c-torture and the
# template's programs run on.
{ stdenv, cpu, description }:

stdenv.mkDerivation {
  pname = "${cpu}-sim";
  version = "0";
  src = ../tools/${cpu}-sim;
  installPhase = "install -Dm755 ${cpu}-sim -t $out/bin";
  meta.description = description;
  meta.mainProgram = "${cpu}-sim";
}
