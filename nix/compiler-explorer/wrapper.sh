# Run Compiler Explorer with the m6809 configuration.
# @ce@ and @etc@ are filled in by flake.nix.
#
# CE wants to write next to its config (short-link storage), so build a
# writable root in the state directory: symlinks to the packaged config plus
# generated settings. Other *.local.properties files put in
# $state/etc/config override the packaged ones.
state="${XDG_STATE_HOME:-$HOME/.local/state}/compiler-explorer-m6809"
mkdir -p "$state/etc/config" "$state/storage"
find "$state/etc/config" -maxdepth 1 -type l -delete
ln -s @etc@/config/* "$state/etc/config/"
cat > "$state/etc/config/compiler-explorer.local.properties" <<PROPS
# Generated on each start; changes here are lost.
localStorageFolder=$state/storage
PROPS

# Rust needs an m6809 rustc, which is not built by nix yet (TODO). Use
# $M6809_RUSTC, or the stage 1 build in third_party/rust when this is run
# from the top of the repository.
languages=(c c++ llvm)
if [ -z "${M6809_RUSTC:-}" ]; then
  for r in "$PWD"/third_party/rust/build/*/stage1/bin/rustc; do
    if [ -x "$r" ]; then M6809_RUSTC=$r; break; fi
  done
fi
if [ -n "${M6809_RUSTC:-}" ]; then
  export M6809_RUSTC
  languages+=(rust)
  echo "compiler-explorer-m6809: Rust from $M6809_RUSTC" >&2
else
  echo "compiler-explorer-m6809: no Rust; set M6809_RUSTC or run from the" \
    "repository with a stage 1 build in third_party/rust" >&2
fi

# --tmp-dir also stops CE using the Windows %TEMP% under WSL.
exec @ce@ \
  --root-dir "$state/etc" --env m6809 --language "${languages[@]}" \
  --tmp-dir "${TMPDIR:-/tmp}" "$@"
