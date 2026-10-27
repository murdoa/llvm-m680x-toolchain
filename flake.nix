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

    # The Solartron workbench (emulator, scenarios, golden traces), read
    # only, and the original ROM dumps, which it keeps outside git. The
    # firmware's differential and acceptance tests run against them. On
    # another machine: --override-input solartron-workbench <path> (and
    # solartron-roms).
    solartron-workbench = {
      url = "git+file:../solartron";
      flake = false;
    };
    solartron-roms = {
      url = "path:./Solartron%207081%20ROMs";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, llvm-project, compiler-explorer
            , solartron-workbench, solartron-roms }:
    let
      forAllSystems = nixpkgs.lib.genAttrs [ "aarch64-linux" "x86_64-linux" ];
    in {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          inherit (self.packages.${system}) llvm-m6809 m6809-toolchain m6809-sim m6800-sim;

          # The workbench with its ROM dumps where it looks for them.
          workbench = pkgs.runCommand "solartron-workbench" { } ''
            cp -r ${solartron-workbench} $out
            chmod -R u+w $out
            cp -r ${solartron-roms} "$out/Solartron 7081 ROMs"
          '';

          simulator = cpu: desc: pkgs.stdenv.mkDerivation {
            pname = "${cpu}-sim";
            version = "0";
            src = ./tools/${cpu}-sim;
            installPhase = "install -Dm755 ${cpu}-sim -t $out/bin";
            meta.description = desc;
            meta.mainProgram = "${cpu}-sim";
          };
        in {
          # clang, lld, llc, llvm-mc etc. with only the M6809 and M6800 targets.
          # Built with clang like configure.sh does. With GCC, LLVM turns off
          # precompiled headers, which the backend relies on for a few
          # missing #includes.
          llvm-m6809 = pkgs.clangStdenv.mkDerivation {
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

          # llvm-m6809 plus the runtime for both targets (crt0, compiler-rt
          # builtins, libc, libc++ and the simulator board libraries), so
          # that `clang --target=m6809 x.c`, `clang++ -std=c++26` and
          # `--target=m6800` link. clang finds the runtime relative to its
          # own binary, so the tools are copied rather than symlinked.
          m6809-toolchain = pkgs.stdenv.mkDerivation {
            pname = "m6809-toolchain";
            inherit (llvm-m6809) version;
            src = ./runtime;

            # cmake generates the libc++ headers; the runtime Makefile runs it.
            nativeBuildInputs = with pkgs; [ cmake ninja python3 ];
            dontConfigure = true;

            buildPhase = ''
              runHook preBuild
              mkdir -p $out/bin $out/lib
              for t in clang clang++ clang-cpp ld.lld lld llc llvm-mc llvm-ar \
                       llvm-ranlib llvm-objdump llvm-objcopy llvm-nm llvm-size \
                       llvm-readobj llvm-readelf llvm-cxxfilt; do
                cp -P ${llvm-m6809}/bin/$t $out/bin/
              done
              # clang and friends are symlinks to clang-NN; copy that too.
              cp ${llvm-m6809}/bin/clang-[0-9]* $out/bin/
              cp -r ${llvm-m6809}/lib/clang $out/lib/
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
          };

          # New firmware for both boards of the Solartron 7081, as EPROM
          # images: earthy/ holds earthy.bin (6000-FFFF) and one file per
          # 8 KB chip, floating/ holds floating.bin (E000-FFFF).
          solartron-firmware = pkgs.stdenv.mkDerivation {
            pname = "solartron-firmware";
            version = "0";
            src = pkgs.lib.cleanSourceWith {
              src = ./firmware;
              filter = path: type: baseNameOf path != "build";
            };
            nativeBuildInputs = with pkgs; [ cmake ninja python3 ];
            dontConfigure = true;
            buildPhase = ''
              runHook preBuild
              for board in earthy:m6809 floating:m6800; do
                b=''${board%%:*}
                cmake -G Ninja -S . -B build/$b -DBOARD=$b \
                  -DCMAKE_TOOLCHAIN_FILE=cmake/''${board##*:}.cmake \
                  -DM6809_TOOLCHAIN=${m6809-toolchain}
                ninja -C build/$b
              done
              runHook postBuild
            '';
            # The register and image size checks (10% spare) and the number
            # routines' differential test against the original ROM.
            doCheck = true;
            checkPhase = ''
              runHook preCheck
              export SOLARTRON_WORKBENCH=${workbench}
              for b in earthy floating; do
                ctest --test-dir build/$b --output-on-failure
              done
              runHook postCheck
            '';
            installPhase = ''
              runHook preInstall
              for b in earthy floating; do
                install -Dm644 -t $out/$b build/$b/image/*.bin
              done
              runHook postInstall
            '';
            dontStrip = true;
            meta.description = "C++26 firmware for the Solartron 7081 earthy and floating boards";
          };

          solartron-workbench = workbench;

          # The simulators the runtime tests and c-torture run on.
          m6809-sim = simulator "m6809" "6809 and HD6309 simulator for test programs";
          m6800-sim = simulator "m6800" "HD6301 simulator for test programs";

          compiler-explorer = pkgs.callPackage ./nix/compiler-explorer/package.nix {
            src = compiler-explorer;
          };

          # Compiler Explorer with only the m6809 and m6800 compilers configured.
          # Extra arguments go to compiler-explorer, e.g. --port 8080.
          compiler-explorer-m6809 =
            let
              ce = self.packages.${system}.compiler-explorer;
              # Runs exe without the flags CE adds for host tools that ours
              # reject: the x86 syntax flag of its Intel-syntax filter (for
              # objdump, llc and rustc) and GNU c++filt's --no-verbose.
              ceTool = name: exe: pkgs.writeShellScript name ''
                args=()
                for a in "$@"; do
                  case $a in
                    *x86-asm-syntax=*|--no-verbose) ;;
                    *) args+=("$a") ;;
                  esac
                done
                exec ${exe} "''${args[@]}"
              '';
              objdump = ceTool "m6809-objdump" "${m6809-toolchain}/bin/llvm-objdump";
              llc = ceTool "m6809-llc" "${m6809-toolchain}/bin/llc";
              cxxfilt = ceTool "m6809-cxxfilt" "${m6809-toolchain}/bin/llvm-cxxfilt";
              # TODO: build rustc in nix. See rustc.sh.
              rustc = pkgs.writeShellScript "m6809-rustc"
                (builtins.replaceStrings [ "@rt@" ] [ "${./nix/compiler-explorer/rust-rt}" ]
                  (builtins.readFile ./nix/compiler-explorer/rustc.sh));
              rootDir = pkgs.runCommand "compiler-explorer-m6809-etc" { } ''
                cp -r ${ce}/share/compiler-explorer/etc $out
                chmod -R u+w $out
                for f in ${./nix/compiler-explorer}/*.properties; do
                  substitute $f $out/config/$(basename $f) \
                    --replace-quiet @toolchain@ ${m6809-toolchain} \
                    --replace-quiet @m6809sim@ ${m6809-sim} \
                    --replace-quiet @m6800sim@ ${m6800-sim} \
                    --replace-quiet @rustc@ ${rustc} \
                    --replace-quiet @llc@ ${llc} \
                    --replace-quiet @cxxfilt@ ${cxxfilt} \
                    --replace-quiet @objdump@ ${objdump}
                done
              '';
            in pkgs.writeShellApplication {
              name = "compiler-explorer-m6809";
              runtimeInputs = [ pkgs.coreutils pkgs.findutils pkgs.git ];
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

      checks = forAllSystems (system:
        let pkgs = nixpkgs.legacyPackages.${system};
        in {
          inherit (self.packages.${system}) solartron-firmware;
          # Every acceptance scenario on the emulated instrument: both new
          # images together and each with the other board's original,
          # against the golden traces (with the BUGS.md differences).
          # On the Python model: the pinned workbench's emu-rs has no
          # clocked ADC (accept --adc-schedule) yet, and the sandbox has no
          # cargo to build it.
          firmware-acceptance = pkgs.runCommand "firmware-acceptance" {
            nativeBuildInputs = [ pkgs.python3 ];
            SOLARTRON_WORKBENCH = self.packages.${system}.solartron-workbench;
          } ''
            fw=${self.packages.${system}.solartron-firmware}
            cd ${pkgs.lib.cleanSourceWith {
              src = ./firmware;
              filter = path: type: baseNameOf path != "build";
            }}
            python3 -I -B tests/acceptance/run.py \
              --earthy $fw/earthy/earthy.bin --floating $fw/floating/floating.bin \
              --pairs both,earthy,floating --engine python -j $NIX_BUILD_CORES
            touch $out
          '';
          # The firmware's portable modules, on the build machine with the
          # address and undefined-behaviour sanitizers.
          firmware-host-tests = pkgs.clangStdenv.mkDerivation {
            name = "firmware-host-tests";
            src = pkgs.lib.cleanSourceWith {
              src = ./firmware;
              filter = path: type: baseNameOf path != "build";
            };
            nativeBuildInputs = with pkgs; [ cmake ninja python3 ];
            cmakeFlags = [ "-DBOARD=host" ];
            doCheck = true;
            checkPhase = "ctest --output-on-failure";
            installPhase = "touch $out";
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
