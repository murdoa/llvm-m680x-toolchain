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
          inherit (self.packages.${system}) llvm-m6809 m6809-toolchain m6809-sim;
        in {
          # clang, lld, llc, llvm-mc etc. with only the M6809 and M6800 targets.
          llvm-m6809 = pkgs.stdenv.mkDerivation {
            pname = "llvm-m6809";
            version = "unstable-${llvm-project.lastModifiedDate}";
            src = llvm-project;
            sourceRoot = "source/llvm";

            nativeBuildInputs = with pkgs; [ cmake ninja python3 ];

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
          };

          # llvm-m6809 plus the runtime (crt0, compiler-rt builtins, libc and
          # the m6809-sim board library), so that `clang --target=m6809 x.c`
          # links. clang finds the runtime relative to its own binary, so the
          # tools are copied rather than symlinked.
          m6809-toolchain = pkgs.stdenv.mkDerivation {
            pname = "m6809-toolchain";
            inherit (llvm-m6809) version;
            src = ./runtime;

            dontConfigure = true;

            buildPhase = ''
              runHook preBuild
              mkdir -p $out/bin $out/lib
              for t in clang clang++ clang-cpp ld.lld lld llc llvm-mc llvm-ar \
                       llvm-ranlib llvm-objdump llvm-objcopy llvm-nm llvm-size \
                       llvm-readelf llvm-cxxfilt; do
                cp -P ${llvm-m6809}/bin/$t $out/bin/
              done
              # clang and friends are symlinks to clang-NN; copy that too.
              cp ${llvm-m6809}/bin/clang-[0-9]* $out/bin/
              cp -r ${llvm-m6809}/lib/clang $out/lib/
              chmod -R u+w $out

              make install LLVM_BUILD=$out LLVM_SRC=${llvm-project} \
                -j$NIX_BUILD_CORES
              runHook postBuild
            '';

            dontInstall = true;

            meta.description = "clang toolchain and runtime for the M6809";
          };

          # The 6809 simulator the runtime tests and c-torture run on.
          m6809-sim = pkgs.stdenv.mkDerivation {
            pname = "m6809-sim";
            version = "0";
            src = ./tools/m6809-sim;
            installPhase = "install -Dm755 m6809-sim -t $out/bin";
            meta.description = "6809 simulator for running compiled test programs";
            meta.mainProgram = "m6809-sim";
          };

          compiler-explorer = pkgs.callPackage ./nix/compiler-explorer/package.nix {
            src = compiler-explorer;
          };

          # Compiler Explorer with only the m6809 compilers configured.
          # Extra arguments go to compiler-explorer, e.g. --port 8080.
          compiler-explorer-m6809 =
            let
              ce = self.packages.${system}.compiler-explorer;
              rootDir = pkgs.runCommand "compiler-explorer-m6809-etc" { } ''
                cp -r ${ce}/share/compiler-explorer/etc $out
                chmod -R u+w $out
                for f in ${./nix/compiler-explorer}/*.properties; do
                  substitute $f $out/config/$(basename $f) \
                    --replace-quiet @toolchain@ ${m6809-toolchain} \
                    --replace-quiet @sim@ ${m6809-sim}
                done
              '';
            in pkgs.writeShellApplication {
              name = "compiler-explorer-m6809";
              runtimeInputs = [ pkgs.coreutils pkgs.findutils ];
              text = builtins.replaceStrings [ "@ce@" "@etc@" ]
                [ (pkgs.lib.getExe ce) "${rootDir}" ]
                (builtins.readFile ./nix/compiler-explorer/wrapper.sh);
            };

          default = m6809-toolchain;
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
