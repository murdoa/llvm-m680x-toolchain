#![no_std]
// A panic stops the program: the simulator runs until its instruction limit.
#[panic_handler]
fn panic(_: &core::panic::PanicInfo) -> ! { loop {} }
