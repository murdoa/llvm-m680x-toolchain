//! A no_std smoke test: crt0 calls `main`, which returns 0 when every
//! check passes and a nonzero code naming the first failure otherwise.
#![no_std]

use core::hint::black_box;

#[panic_handler]
fn panic(_: &core::panic::PanicInfo) -> ! {
    unsafe extern "C" {
        fn abort() -> !;
    }
    unsafe { abort() }
}

#[derive(Clone, Copy, PartialEq, Debug)]
struct Point {
    x: i16,
    y: i16,
}

#[inline(never)]
fn add(a: Point, b: Point) -> Point {
    Point { x: a.x + b.x, y: a.y + b.y }
}

#[inline(never)]
fn fib(n: u32) -> u32 {
    if n < 2 { n } else { fib(n - 1) + fib(n - 2) }
}

fn checksum(data: &[u8]) -> u16 {
    data.iter().fold(0u16, |acc, &b| acc.rotate_left(3) ^ b as u16)
}

trait Shape {
    fn area(&self) -> u32;
}
struct Rect(u16, u16);
struct Square(u16);
impl Shape for Rect {
    fn area(&self) -> u32 { self.0 as u32 * self.1 as u32 }
}
impl Shape for Square {
    fn area(&self) -> u32 { self.0 as u32 * self.0 as u32 }
}

fn total(shapes: &[&dyn Shape]) -> u32 {
    shapes.iter().map(|s| s.area()).sum()
}

fn parse(s: &str) -> Option<u16> {
    let mut n: u16 = 0;
    for c in s.bytes() {
        let d = (c as char).to_digit(10)?;
        n = n.checked_mul(10)?.checked_add(d as u16)?;
    }
    Some(n)
}

#[unsafe(no_mangle)]
pub extern "C" fn main() -> i16 {
    let p = add(black_box(Point { x: 3, y: -4 }), black_box(Point { x: 10, y: 20 }));
    if p != (Point { x: 13, y: 16 }) {
        return 1;
    }
    if fib(black_box(15)) != 610 {
        return 2;
    }
    if checksum(black_box(b"Solartron 7081")) != 0xe78b {
        return 3;
    }
    let shapes: [&dyn Shape; 2] = [&Rect(300, 400), &Square(black_box(250))];
    if total(&shapes) != 182_500 {
        return 4;
    }
    if parse(black_box("65535")) != Some(65535) || parse(black_box("65536")).is_some() {
        return 5;
    }
    let x: u64 = black_box(0x0123_4567_89ab_cdef);
    if x.wrapping_mul(3) / 7 != 0x007c_d49a_166e_33af {
        return 6;
    }
    let f: f32 = black_box(2.5);
    if (f * f) as i16 != 6 {
        return 7;
    }
    0
}
