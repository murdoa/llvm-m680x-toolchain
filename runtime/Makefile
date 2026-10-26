# Runtime for the m6809 and m6800 targets: crt0, the compiler-rt builtins, a
# small libc, libc++ and board support for the simulators.
#
#   make            build into ./out/m6809
#   make install    copy into the LLVM build so that plain
#                   "clang --target=m6809 x.c" finds everything
#   make TARGET=m6800 install
#                   the same for the 6800
#
# LLVM_BUILD is the LLVM build directory with clang, lld and llvm-ar.
# libc++ needs cmake, ninja and python to generate its headers.

TARGET ?= m6809

LLVM_BUILD ?= $(abspath ../build)
LLVM_SRC ?= $(abspath ../llvm-project)
CC := $(LLVM_BUILD)/bin/clang
AR := $(LLVM_BUILD)/bin/llvm-ar
OUT := out/$(TARGET)

CFLAGS := --target=$(TARGET) -O2 -ffreestanding -Wall -Werror
LIBC_CFLAGS := $(CFLAGS) -fno-builtin -Ilibc/include
LIBC_CXXFLAGS := $(LIBC_CFLAGS) -fno-exceptions -fno-rtti

RESOURCE_DIR := $(shell $(CC) --target=$(TARGET) -print-resource-dir)
SYSROOT := $(LLVM_BUILD)/lib/clang-runtimes/$(TARGET)

# compiler-rt: the target's assembly, plus the generic C sources a 16-bit target
# can use (no 128-bit types, no OS support).
CRT_DIR := $(LLVM_SRC)/compiler-rt/lib/builtins
# The frame pointer word of functions with variable-sized objects.
CRT_ASM_m6800 := fp
CRT_ASM := mulhi3 divmodhi4 mulsi3 shiftsi3 $(CRT_ASM_$(TARGET))
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
CRT_OBJS := $(CRT_ASM:%=$(OUT)/crt/$(TARGET)_%.o) $(CRT_C:%=$(OUT)/crt/%.o)

# One function (or a few that belong together) per file, so that a program
# defining its own memcpy or abs does not clash with the archive's copy.
LIBC_SRCS := $(wildcard libc/*.c libc/*/*.c)
# The C++ ABI support and operator new/delete.
LIBC_CXX_SRCS := $(wildcard libc/cxx/*.cpp)
# The target's own parts of libc (setjmp), next to its crt0.
LIBC_ASM := $(filter-out arch/$(TARGET)/crt0.S,$(wildcard arch/$(TARGET)/*.S))
LIBC_OBJS := $(LIBC_SRCS:libc/%.c=$(OUT)/libc/%.o) \
  $(LIBC_CXX_SRCS:libc/%.cpp=$(OUT)/libc/%.o) \
  $(LIBC_ASM:arch/$(TARGET)/%.S=$(OUT)/libc/%.o)

# libc++: cmake generates the headers, and the parts of the compiled library
# that freestanding programs use are built here. The rest of the library
# needs threads, locales or files, or does not build with a 16-bit int.
CXX := $(LLVM_BUILD)/bin/clang++
LIBCXX_BUILD := $(abspath $(OUT)/libcxx-build)
LIBCXX_INC := $(LIBCXX_BUILD)/include/c++/v1
LIBCXX_SRC := $(LLVM_SRC)/libcxx/src
LIBCXX_PARTS := any bind exception expected functional optional variant
# Replacements for parts of libc++, in libcxx/.
LIBCXX_OWN := verbose_abort
# std::sort of each built-in type in an object of its own, in place of
# algorithm.cpp (see libcxx/sort.cpp). An underscore stands for a space.
SORT_TYPES := char signed_char unsigned_char short unsigned_short int \
  unsigned long unsigned_long long_long unsigned_long_long float double \
  long_double
LIBCXX_OBJS := $(LIBCXX_PARTS:%=$(OUT)/libcxx/%.o) \
  $(LIBCXX_OWN:%=$(OUT)/libcxx/own-%.o) \
  $(SORT_TYPES:%=$(OUT)/libcxx/sort-%.o)
# exception.cpp warns, with #warning, that exception_ptr needs an ABI library.
HASH := \#
LIBCXX_CXXFLAGS := --target=$(TARGET) -O2 -std=c++26 -Wall -Werror \
  -Wno-$(HASH)warnings \
  -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti \
  -D_LIBCPP_BUILDING_LIBRARY -D_LIBCPP_BUILDING_HAS_NO_ABI_LIBRARY \
  -D_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER -D_LIBCPP_REMOVE_TRANSITIVE_INCLUDES \
  -nostdinc++ -I$(LIBCXX_SRC) -isystem $(LIBCXX_INC) -isystem libc/include

all: $(OUT)/crt0.o $(OUT)/libclang_rt.builtins.a $(OUT)/libc.a \
  $(OUT)/libm.a $(OUT)/lib$(TARGET)sim.a $(OUT)/libc++.a

$(OUT)/crt0.o: arch/$(TARGET)/crt0.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/crt/$(TARGET)_%.o: $(CRT_DIR)/$(TARGET)/%.S
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

$(OUT)/libc/%.o: libc/%.cpp $(wildcard libc/include/*.h)
	@mkdir -p $(@D)
	$(CC) $(LIBC_CXXFLAGS) -c $< -o $@

$(OUT)/libc/%.o: arch/$(TARGET)/%.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/libc.a: $(LIBC_OBJS)
	rm -f $@
	$(AR) rcs $@ $^

# The math functions are in libc.a; libm.a is empty so that -lm works.
$(OUT)/libm.a:
	@mkdir -p $(@D)
	rm -f $@
	$(AR) rcs $@

$(OUT)/sim/sim.o: sim/sim.c
	@mkdir -p $(@D)
	$(CC) $(LIBC_CFLAGS) -c $< -o $@

$(OUT)/lib$(TARGET)sim.a: $(OUT)/sim/sim.o
	rm -f $@
	$(AR) rcs $@ $^

# Ninja regenerates the headers when the libc++ sources change.
$(LIBCXX_BUILD)/build.ninja:
	cmake -G Ninja -S $(LLVM_SRC)/runtimes -B $(LIBCXX_BUILD) \
	  -DLLVM_ENABLE_RUNTIMES=libcxx \
	  -DCMAKE_SYSTEM_NAME=Generic \
	  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
	  -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) \
	  -DCMAKE_C_COMPILER_TARGET=$(TARGET) \
	  -DCMAKE_CXX_COMPILER_TARGET=$(TARGET) \
	  -DCMAKE_AR=$(AR) \
	  -DLIBCXX_CXX_ABI=none \
	  -DLIBCXX_ENABLE_SHARED=OFF \
	  -DLIBCXX_ENABLE_EXCEPTIONS=OFF -DLIBCXX_ENABLE_RTTI=OFF \
	  -DLIBCXX_ENABLE_THREADS=OFF -DLIBCXX_ENABLE_MONOTONIC_CLOCK=OFF \
	  -DLIBCXX_ENABLE_LOCALIZATION=OFF -DLIBCXX_ENABLE_FILESYSTEM=OFF \
	  -DLIBCXX_ENABLE_RANDOM_DEVICE=OFF -DLIBCXX_ENABLE_UNICODE=OFF \
	  -DLIBCXX_ENABLE_WIDE_CHARACTERS=OFF \
	  -DLIBCXX_ENABLE_TIME_ZONE_DATABASE=OFF \
	  -DLIBCXX_ENABLE_NEW_DELETE_DEFINITIONS=OFF \
	  -DLIBCXX_ENABLE_VENDOR_AVAILABILITY_ANNOTATIONS=OFF \
	  -DLIBCXX_INCLUDE_BENCHMARKS=OFF -DLIBCXX_INCLUDE_TESTS=OFF \
	  -DLIBCXX_INSTALL_MODULES=OFF >/dev/null

libcxx-headers: $(LIBCXX_BUILD)/build.ninja
	ninja -C $(LIBCXX_BUILD) generate-cxx-headers >/dev/null

$(OUT)/libcxx/%.o: $(LIBCXX_SRC)/%.cpp libcxx-headers
	@mkdir -p $(@D)
	$(CXX) $(LIBCXX_CXXFLAGS) -c $< -o $@

$(OUT)/libcxx/own-%.o: libcxx/%.cpp libcxx-headers
	@mkdir -p $(@D)
	$(CXX) $(LIBCXX_CXXFLAGS) -c $< -o $@

$(OUT)/libcxx/sort-%.o: libcxx/sort.cpp libcxx-headers
	@mkdir -p $(@D)
	$(CXX) $(LIBCXX_CXXFLAGS) -D'TYPE=$(subst _, ,$*)' -c $< -o $@

$(OUT)/libc++.a: $(LIBCXX_OBJS)
	rm -f $@
	$(AR) rcs $@ $(filter %.o,$^)

install: all
	mkdir -p $(SYSROOT)/lib $(SYSROOT)/include $(RESOURCE_DIR)/lib/$(TARGET)
	cp $(OUT)/crt0.o $(OUT)/libc.a $(OUT)/libm.a $(OUT)/lib$(TARGET)sim.a \
	  sim/$(TARGET)-sim.ld \
	  $(SYSROOT)/lib/
	cp libc/include/*.h $(SYSROOT)/include/
	cp $(OUT)/libc++.a $(SYSROOT)/lib/
	rm -rf $(SYSROOT)/include/c++
	mkdir -p $(SYSROOT)/include/c++
	cp -r $(LIBCXX_INC) $(SYSROOT)/include/c++/v1
	cp $(OUT)/libclang_rt.builtins.a $(RESOURCE_DIR)/lib/$(TARGET)/
	cp sim/$(TARGET)-sim.cfg $(LLVM_BUILD)/bin/

# Each test prints its result and exits 0 when it passes. TEST_CPU picks a
# CPU, such as TEST_CPU=-mcpu=6309.
SIM := $(abspath ../tools/$(TARGET)-sim/$(TARGET)-sim)
SIM_FLAGS := $(if $(filter -mcpu=6309,$(TEST_CPU)),-6309)
TESTS := $(notdir $(wildcard tests/*.c tests/*.cpp))

check: install
	@fail=0; for t in $(TESTS); do \
	  case $$t in *.cpp) cc="$(CXX) -std=c++26";; *) cc=$(CC);; esac; \
	  $$cc --target=$(TARGET) $(TEST_CPU) -O2 tests/$$t \
	    -T $(TARGET)-sim.ld -l$(TARGET)sim \
	    -o $(OUT)/$$t.elf || { fail=1; continue; }; \
	  if $(SIM) $(SIM_FLAGS) -max 500000000 $(OUT)/$$t.elf; then :; else \
	    echo "$$t: exit $$?"; fail=1; fi; \
	done; exit $$fail

clean:
	rm -rf $(OUT)

.PHONY: all install check clean libcxx-headers
