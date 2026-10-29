# Changelog

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## Unreleased

The repository replaces `llvm-backend`, keeping its history.

### Added

- `m680x-toolchain` package (the default), with `llvm-m6809` and
  `m6809-toolchain` kept as aliases.
- `overlays.default`, and `apps` for clang, clang++, both simulators and
  Compiler Explorer.
- `nix flake check` runs the runtime tests on the 6809, HD6309 and HD6301
  simulators and builds the template.
- `templates/hello` for `nix flake init -t`.
- Documentation in `docs/`, the licence, and CI.

### Changed

- The simulators' `difftest` target needs `PYCORE` to be given.
