#![no_std]
#![feature(lang_items)]
// Lets a crate use a plain `fn main()`: rustc emits the C `main` that crt0
// calls, which calls this.
#[lang = "start"]
fn start<T>(main: fn() -> T, _argc: isize, _argv: *const *const u8, _sigpipe: u8) -> isize {
    main();
    0
}
