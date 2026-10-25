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
  };

  outputs = { self, nixpkgs, llvm-project }:
    let
      forAllSystems = nixpkgs.lib.genAttrs [ "aarch64-linux" "x86_64-linux" ];
    in {
      packages = forAllSystems (system:
        let pkgs = nixpkgs.legacyPackages.${system}; in {
          # llc, llvm-mc, opt etc. with only the M6809 target enabled.
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
              "-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=M6809"
              "-DLLVM_INCLUDE_BENCHMARKS=OFF"
              "-DLLVM_INCLUDE_EXAMPLES=OFF"
              "-DLLVM_INCLUDE_TESTS=OFF"
              "-DLLVM_PARALLEL_LINK_JOBS=2"
            ];

            meta.description = "LLVM tools with the experimental M6809 backend";
          };

          default = self.packages.${system}.llvm-m6809;
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
