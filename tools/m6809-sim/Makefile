CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra

m6809-sim: m6809-sim.c
	$(CC) $(CFLAGS) -o $@ $<

# Cross-check against the Python 6809 core in solartron-emu.
PYCORE ?= ../../../solartron/bench
difftest: m6809-sim
	python3 difftest.py $(PYCORE) 2000 1
	ONE=1 python3 difftest.py $(PYCORE) 4000 2

clean:
	rm -f m6809-sim

.PHONY: difftest clean
