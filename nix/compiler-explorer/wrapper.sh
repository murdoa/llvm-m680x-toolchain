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

# --tmp-dir also stops CE using the Windows %TEMP% under WSL.
exec @ce@ \
  --root-dir "$state/etc" --env m6809 --language c c++ llvm \
  --tmp-dir "${TMPDIR:-/tmp}" "$@"
