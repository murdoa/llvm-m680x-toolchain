# rustc for Compiler Explorer on m6809-none-elf and m6800-none-elf.
# @rt@ (the rust-rt sources) is filled in by flake.nix.
#
# TODO: build rustc in nix. Until then this runs the rustc in $M6809_RUSTC,
# which the launcher points at the stage 1 build in third_party/rust.
#
# So that ordinary Rust compiles and runs, it adds, unless the source
# already does it itself:
#   -Zcrate-attr=no_std   (unless the source says #![no_std])
#   ce_panic              a #[panic_handler] (unless it has one)
#   ce_start              the start lang item, so a plain fn main() works
# The two crates are built with this rustc into $M6809_RUST_RT and rebuilt
# when rustc changes.
rustc=${M6809_RUSTC:?is not set}
rtdir=${M6809_RUST_RT:?is not set}

args=() src= target=
prev=
for a in "$@"; do
  case $a in
    # CE adds this for its Intel-syntax filter; it is x86 only.
    -Cllvm-args=--x86-asm-syntax=*) ;;
    *) args+=("$a") ;;
  esac
  case $a in
    *.rs) src=$a ;;
    --target=*) target=${a#--target=} ;;
  esac
  [ "$prev" = --target ] && target=$a
  prev=$a
done

# Version checks and the like: no source, nothing to add.
if [ -z "$src" ] || [ -z "$target" ]; then
  exec "$rustc" "${args[@]}"
fi

rt=$rtdir/$target
for crate in ce_start ce_panic; do
  lib=$rt/lib$crate.rlib
  if [ ! -e "$lib" ] || [ "$rustc" -nt "$lib" ]; then
    mkdir -p "$rt"
    tmp=$(mktemp -d "$rt/build.XXXXXX")
    "$rustc" --target "$target" -C opt-level=2 -C panic=abort \
      -A internal_features --crate-type rlib @rt@/$crate.rs \
      -o "$tmp/lib$crate.rlib" >&2 || exit 1
    mv -f "$tmp/lib$crate.rlib" "$lib"
    rmdir "$tmp"
  fi
done

extra=(-Zunstable-options --extern "force:ce_start=$rt/libce_start.rlib")
grep -q '#!\[no_std\]' "$src" || extra+=(-Zcrate-attr=no_std)
grep -q 'panic_handler' "$src" ||
  extra+=(--extern "force:ce_panic=$rt/libce_panic.rlib")

exec "$rustc" "${extra[@]}" "${args[@]}"
