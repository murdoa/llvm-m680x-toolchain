CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra

m6800-sim: m6800-sim.c
	$(CC) $(CFLAGS) -o $@ $<

# Cross-check against the Python HD6301 core in the Solartron workbench:
#   make difftest PYCORE=<solartron checkout>/bench
difftest: m6800-sim
	@test -n "$(PYCORE)" || { echo "set PYCORE to the directory holding emu/hd6301.py"; exit 1; }
	python3 -B difftest.py $(PYCORE) 2000 1
	ONE=1 python3 -B difftest.py $(PYCORE) 4000 2

clean:
	rm -f m6800-sim

.PHONY: difftest clean
