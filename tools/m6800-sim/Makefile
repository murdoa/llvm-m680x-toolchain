CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra

m6800-sim: m6800-sim.c
	$(CC) $(CFLAGS) -o $@ $<

# Cross-check against the Python HD6301 core in solartron.
PYCORE ?= ../../../solartron/bench
difftest: m6800-sim
	python3 -B difftest.py $(PYCORE) 2000 1
	ONE=1 python3 -B difftest.py $(PYCORE) 4000 2

clean:
	rm -f m6800-sim

.PHONY: difftest clean
