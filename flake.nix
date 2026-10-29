{
  description = "C, C++ and Rust for the Motorola 6809, Hitachi HD6309 and HD6301/HD6303, on LLVM";

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
          # LLVM with the M6809 and M6800 targets: clang, lld and the tools.
          llvm = pkgs.callPackage ./nix/llvm.nix { inherit llvm-project; };
          # That plus the runtime for both targets: what most users want.
          m680x-toolchain = pkgs.callPackage ./nix/toolchain.nix {
            inherit llvm-project;
            inherit (p) llvm;
            pname = "m680x-toolchain";
          };
          # The names before the repository was renamed.
          llvm-m6809 = p.llvm;
          m6809-toolchain = p.m680x-toolchain;

          m6809-sim = simulator "m6809" "6809 and HD6309 simulator for test programs";
          m6800-sim = simulator "m6800" "HD6301 simulator for test programs";

          compiler-explorer = pkgs.callPackage ./nix/compiler-explorer/package.nix {
            src = compiler-explorer;
          };
          compiler-explorer-m6809 = pkgs.callPackage ./nix/compiler-explorer/m680x.nix {
            inherit (p) compiler-explorer m6809-sim m6800-sim;
            toolchain = p.m680x-toolchain;
          };

          default = p.m680x-toolchain;
        });

      # The toolchain and simulators in another flake's nixpkgs:
      # nixpkgs.overlays = [ llvm-m680x-toolchain.overlays.default ].
      overlays.default = final: prev: {
        inherit (self.packages.${final.stdenv.hostPlatform.system})
          m680x-toolchain m6809-sim m6800-sim;
      };

      apps = forAllSystems (system:
        let
          p = self.packages.${system};
          app = drv: exe: { type = "app"; program = "${drv}/bin/${exe}"; };
        in {
          # nix run .#clang -- --target=m6809 hello.c
          clang = app p.m680x-toolchain "clang";
          "clang++" = app p.m680x-toolchain "clang++";
          m6809-sim = app p.m6809-sim "m6809-sim";
          m6800-sim = app p.m6800-sim "m6800-sim";
          # nix run .#compiler-explorer, then open http://localhost:10240
          compiler-explorer = app p.compiler-explorer-m6809 "compiler-explorer-m6809";
          default = app p.m680x-toolchain "clang";
        });

      checks = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          p = self.packages.${system};
        in import ./nix/checks.nix {
          inherit (pkgs) runCommand gnumake;
          inherit (p) m6809-sim m6800-sim;
          toolchain = p.m680x-toolchain;
        } // {
          inherit (p) compiler-explorer-m6809;
        });

      # nix flake init -t github:murdoa/llvm-m680x-toolchain
      templates.default = {
        path = ./templates/hello;
        description = "A C program for the 6809 and HD6301, run in the simulators";
      };

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
