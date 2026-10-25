{
  description = "LLVM backends for the Motorola 6809 / HD6309 and HD6301";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      forAllSystems = nixpkgs.lib.genAttrs [ "aarch64-linux" "x86_64-linux" ];
    in {
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
        # Keep nix's cc-wrapper from adding hardening flags that break some LLVM tests.
        hardeningDisable = [ "all" ];
        };
      });
    };
}
