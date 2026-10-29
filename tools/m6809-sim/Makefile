CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra

m6809-sim: m6809-sim.c
	$(CC) $(CFLAGS) -o $@ $<

# Cross-check against the Python 6809 core in the Solartron workbench:
#   make difftest PYCORE=<solartron checkout>/bench
difftest: m6809-sim
	@test -n "$(PYCORE)" || { echo "set PYCORE to the directory holding emu/mc6809.py"; exit 1; }
	python3 difftest.py $(PYCORE) 2000 1
	ONE=1 python3 difftest.py $(PYCORE) 4000 2

# HD6309 instructions, checked by a self-checking program (needs lwasm).
LWASM ?= ../../third_party/prefix/bin/lwasm
test6309: m6809-sim
	$(LWASM) --6309 --format=raw -o test6309.bin test6309.asm
	./m6809-sim -6309 -load 1000 test6309.bin && echo "test6309: PASS"

clean:
	rm -f m6809-sim test6309.bin

.PHONY: difftest test6309 clean
