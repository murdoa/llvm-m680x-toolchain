{
  description = "A C program for the 6809 and HD6301";

  inputs.m680x.url = "github:murdoa/llvm-m680x-toolchain";

  outputs = { self, m680x }:
    let
      forAllSystems = m680x.inputs.nixpkgs.lib.genAttrs [ "aarch64-linux" "x86_64-linux" ];
    in {
      # nix develop, then: make run   (or make run TARGET=m6800)
      devShells = forAllSystems (system:
        let
          pkgs = m680x.inputs.nixpkgs.legacyPackages.${system};
          m = m680x.packages.${system};
        in {
          default = pkgs.mkShell {
            packages = [ pkgs.gnumake m.m680x-toolchain m.m6809-sim m.m6800-sim ];
          };
        });
    };
}
