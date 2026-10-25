# Runtime for the m6809 target: crt0, the compiler-rt builtins, a small libc
# and board support for m6809-sim.
#
#   make            build into ./out
#   make install    copy into the LLVM build so that plain
#                   "clang --target=m6809 x.c" finds everything
#
# LLVM_BUILD is the LLVM build directory with clang, lld and llvm-ar.

LLVM_BUILD ?= $(abspath ../build)
LLVM_SRC ?= $(abspath ../llvm-project)
CC := $(LLVM_BUILD)/bin/clang
AR := $(LLVM_BUILD)/bin/llvm-ar
OUT := out

CFLAGS := --target=m6809 -O2 -ffreestanding -Wall -Werror
LIBC_CFLAGS := $(CFLAGS) -fno-builtin -Ilibc/include

RESOURCE_DIR := $(shell $(CC) --target=m6809 -print-resource-dir)
SYSROOT := $(LLVM_BUILD)/lib/clang-runtimes/m6809

# compiler-rt: the 6809 assembly, plus the generic C sources a 16-bit target
# can use (no 128-bit types, no OS support).
CRT_DIR := $(LLVM_SRC)/compiler-rt/lib/builtins
CRT_ASM := mulhi3 divmodhi4 mulsi3 shiftsi3
CRT_C := absvdi2 absvsi2 adddf3 addsf3 addvdi3 addvsi3 ashldi3 ashrdi3 \
  bswapdi2 bswapsi2 clzdi2 clzsi2 cmpdi2 comparedf2 comparesf2 ctzdi2 ctzsi2 \
  divdc3 divdf3 divdi3 divmoddi4 divmodsi4 divsc3 divsf3 divsi3 extendsfdf2 \
  extendhfsf2 extendhfdf2 ffsdi2 ffssi2 fixdfdi fixdfsi fixsfdi fixsfsi \
  fixunsdfdi fixunsdfsi fixunssfdi fixunssfsi floatdidf floatdisf floatsidf \
  floatsisf floatundidf floatundisf floatunsidf floatunsisf fp_mode int_util \
  lshrdi3 moddi3 modsi3 muldc3 muldf3 muldi3 mulodi4 mulosi4 mulsc3 mulsf3 \
  mulvdi3 mulvsi3 negdf2 negdi2 negsf2 negvdi2 negvsi2 paritydi2 paritysi2 \
  popcountdi2 popcountsi2 powidf2 powisf2 subdf3 subsf3 subvdi3 subvsi3 \
  truncdfhf2 truncdfsf2 truncsfhf2 ucmpdi2 udivdi3 udivmoddi4 udivmodsi4 \
  udivsi3 umoddi3 umodsi3
CRT_OBJS := $(CRT_ASM:%=$(OUT)/crt/m6809_%.o) $(CRT_C:%=$(OUT)/crt/%.o)

LIBC_SRCS := string stdlib malloc ctype stdio
LIBC_OBJS := $(LIBC_SRCS:%=$(OUT)/libc/%.o)

all: $(OUT)/crt0.o $(OUT)/libclang_rt.builtins.a $(OUT)/libc.a \
  $(OUT)/libm6809sim.a

$(OUT)/crt0.o: crt0.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/crt/m6809_%.o: $(CRT_DIR)/m6809/%.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/crt/%.o: $(CRT_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -fno-builtin -c $< -o $@

$(OUT)/libclang_rt.builtins.a: $(CRT_OBJS)
	rm -f $@
	$(AR) rcs $@ $^

$(OUT)/libc/%.o: libc/%.c $(wildcard libc/include/*.h)
	@mkdir -p $(@D)
	$(CC) $(LIBC_CFLAGS) -c $< -o $@

$(OUT)/libc.a: $(LIBC_OBJS)
	rm -f $@
	$(AR) rcs $@ $^

$(OUT)/sim/sim.o: sim/sim.c
	@mkdir -p $(@D)
	$(CC) $(LIBC_CFLAGS) -c $< -o $@

$(OUT)/libm6809sim.a: $(OUT)/sim/sim.o
	rm -f $@
	$(AR) rcs $@ $^

install: all
	mkdir -p $(SYSROOT)/lib $(SYSROOT)/include $(RESOURCE_DIR)/lib/m6809
	cp $(OUT)/crt0.o $(OUT)/libc.a $(OUT)/libm6809sim.a sim/m6809-sim.ld \
	  $(SYSROOT)/lib/
	cp libc/include/*.h $(SYSROOT)/include/
	cp $(OUT)/libclang_rt.builtins.a $(RESOURCE_DIR)/lib/m6809/

# Each test prints its result and exits 0 when it passes.
SIM := $(abspath ../tools/m6809-sim/m6809-sim)
TESTS := $(basename $(notdir $(wildcard tests/*.c)))

check: install
	@fail=0; for t in $(TESTS); do \
	  $(CC) --target=m6809 -O2 tests/$$t.c -T m6809-sim.ld -lm6809sim \
	    -o $(OUT)/$$t.elf || { fail=1; continue; }; \
	  if $(SIM) -max 500000000 $(OUT)/$$t.elf; then :; else \
	    echo "$$t: exit $$?"; fail=1; fi; \
	done; exit $$fail

clean:
	rm -rf $(OUT)

.PHONY: all install check clean
