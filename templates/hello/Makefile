# Build for the simulator and run it:
#   make run              the 6809 (m6809-sim)
#   make run TARGET=m6800 the HD6301 (m6800-sim)
# --config=<target>-sim.cfg links the simulator's memory map and board
# library; for real hardware, give your own linker script with -T.
TARGET ?= m6809
NAME_m6809 := 6809
NAME_m6800 := HD6301

CC := clang --target=$(TARGET) --config=$(TARGET)-sim.cfg
CFLAGS := -O2 -Wall -Wextra -DTARGET_NAME='"$(NAME_$(TARGET))"'

hello-$(TARGET).elf: hello.c
	$(CC) $(CFLAGS) $< -o $@

run: hello-$(TARGET).elf
	$(TARGET)-sim $<

clean:
	rm -f hello-*.elf

.PHONY: run clean
