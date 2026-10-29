{
  description = "LLVM backends for the Motorola 6809 / HD6309 and HD6301";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    # The backend lives on this branch. For day-to-day work use the local
    # clone in ./llvm-project with configure.sh; this input pins what
    # `nix build` produces. Bump it with `nix flake update llvm-project`.
    llvm-project = {
      url = "github:murdoa/llvm-project/m6809";
      flake = false;
    };

    # Compiler Explorer, run locally by `nix run .#compiler-explorer`.
    # After `nix flake update compiler-explorer`, update npmDepsHash in
    # nix/compiler-explorer/package.nix.
    compiler-explorer = {
      url = "github:compiler-explorer/compiler-explorer";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, llvm-project, compiler-explorer }:
    let
      forAllSystems = nixpkgs.lib.genAttrs [ "aarch64-linux" "x86_64-linux" ];
    in {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          p = self.packages.${system};
          simulator = cpu: description:
            pkgs.callPackage ./nix/simulator.nix { inherit cpu description; };
        in {
          llvm-m6809 = pkgs.callPackage ./nix/llvm.nix { inherit llvm-project; };
          m6809-toolchain = pkgs.callPackage ./nix/toolchain.nix {
            inherit llvm-project;
            llvm = p.llvm-m6809;
          };

          m6809-sim = simulator "m6809" "6809 and HD6309 simulator for test programs";
          m6800-sim = simulator "m6800" "HD6301 simulator for test programs";

          compiler-explorer = pkgs.callPackage ./nix/compiler-explorer/package.nix {
            src = compiler-explorer;
          };
          compiler-explorer-m6809 = pkgs.callPackage ./nix/compiler-explorer/m680x.nix {
            inherit (p) compiler-explorer m6809-sim m6800-sim;
            toolchain = p.m6809-toolchain;
          };

          default = p.m6809-toolchain;
        });

      apps = forAllSystems (system: {
        # nix run .#compiler-explorer, then open http://localhost:10240
        compiler-explorer = {
          type = "app";
          program = nixpkgs.lib.getExe self.packages.${system}.compiler-explorer-m6809;
        };
      });

      devShells = forAllSystems (system:
        let pkgs = nixpkgs.legacyPackages.${system}; in {
          default = pkgs.mkShell {
            packages = with pkgs; [
              cmake
              ninja
              python3
              ccache
              llvmPackages.clang
              llvmPackages.bintools # lld + llvm-ar etc.
              git
            ];
            # Keep nix's cc-wrapper from adding hardening flags that break
            # some LLVM tests.
            hardeningDisable = [ "all" ];
          };
        });
    };
}
