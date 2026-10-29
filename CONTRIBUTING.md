# Contributing

Bug reports and patches are welcome.

## Reporting a bug

A miscompile or crash report is most useful with:

- the `nix flake metadata` revision, or the LLVM fork commit you built;
- the command line (`--target`, `-mcpu`, `-O` level);
- a small C file that shows it, and what you expected. For a wrong result,
  the output of the program in `m6809-sim` or `m6800-sim` and on the host
  helps.

## Where changes go

- **Backends, clang, lld, compiler-rt** live in the LLVM fork,
  [murdoa/llvm-project](https://github.com/murdoa/llvm-project), branch
  `m6809`. Follow the LLVM coding standards and add a lit test for each
  change in code generation or the MC layer. Bump the `llvm-project` input
  here once it is merged.
- **The runtime, simulators, test tools, Nix packaging and docs** live
  here.

See [docs/development.md](docs/development.md) to build and test.

## Before sending a change

- `nix flake check -L` passes.
- For a code-generation change: the lit tests, and c-torture at `-O0`, `-O2`
  and `-Oz` for the CPUs it touches, with no new failures.
- Commit messages follow [Conventional Commits](https://www.conventionalcommits.org/)
  (`fix(runtime): ...`, `docs: ...`), one change per commit.

By contributing you agree that your contribution is under the project's
licence, Apache-2.0 WITH LLVM-exception.
