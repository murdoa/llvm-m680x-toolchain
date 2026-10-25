// m6809-sim: a small Motorola 6809 simulator for running compiled tests.
//
// It loads an ELF executable (or a raw binary), runs it on a flat 64 KiB
// memory, and stops when the program writes its exit code to the exit port.
//
// Test I/O ports:
//   $FF00  write: print the byte on stdout
//   $FF01  write: stop, with the byte as the exit status
//   $FF02  read:  next byte of stdin ($FF at end of input)
//
// Other stops:
//   SWI                    exit status 134 (abort)
//   undefined instruction  exit status 132
//   instruction limit      exit status 124
//
// The cycle counts are the datasheet base cycles plus the indexed-mode
// extras. They are used for comparing code, not for timing peripherals.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { CC_C = 0x01, CC_V = 0x02, CC_Z = 0x04, CC_N = 0x08,
       CC_I = 0x10, CC_H = 0x20, CC_F = 0x40, CC_E = 0x80 };

enum { PORT_PUTCHAR = 0xFF00, PORT_EXIT = 0xFF01, PORT_GETCHAR = 0xFF02 };

enum { STOP_NONE = -1, STOP_ABORT = 134, STOP_ILLEGAL = 132,
       STOP_TIMEOUT = 124 };

typedef struct {
  uint8_t a, b, dp, cc;
  uint16_t x, y, u, s, pc;
  uint8_t mem[0x10000];
  uint64_t cycles, insns;
  int stop;      // STOP_NONE while running, otherwise the exit status
  uint16_t op_pc; // address of the instruction being executed
  int trace;
  int noports;   // treat the test ports as plain memory
} CPU;

// ------------------------------------------------------------------ memory

static uint8_t rd(CPU *c, uint16_t a) {
  if (a == PORT_GETCHAR && !c->noports) {
    int ch = getchar();
    return ch == EOF ? 0xFF : (uint8_t)ch;
  }
  return c->mem[a];
}

static void wr(CPU *c, uint16_t a, uint8_t v) {
  if (c->noports) {
    c->mem[a] = v;
    return;
  }
  if (a == PORT_PUTCHAR) {
    putchar(v);
    return;
  }
  if (a == PORT_EXIT) {
    if (c->stop == STOP_NONE)
      c->stop = v;
    return;
  }
  c->mem[a] = v;
}

static uint16_t rd16(CPU *c, uint16_t a) {
  return (uint16_t)(rd(c, a) << 8 | rd(c, (uint16_t)(a + 1)));
}

static void wr16(CPU *c, uint16_t a, uint16_t v) {
  wr(c, a, (uint8_t)(v >> 8));
  wr(c, (uint16_t)(a + 1), (uint8_t)v);
}

static uint8_t fetch(CPU *c) { return rd(c, c->pc++); }

static uint16_t fetch16(CPU *c) {
  uint16_t v = rd16(c, c->pc);
  c->pc += 2;
  return v;
}

static uint16_t getd(CPU *c) { return (uint16_t)(c->a << 8 | c->b); }

static void setd(CPU *c, uint16_t v) {
  c->a = (uint8_t)(v >> 8);
  c->b = (uint8_t)v;
}

static void push8(CPU *c, uint16_t *sp, uint8_t v) { wr(c, --*sp, v); }

static void push16(CPU *c, uint16_t *sp, uint16_t v) {
  push8(c, sp, (uint8_t)v);
  push8(c, sp, (uint8_t)(v >> 8));
}

static uint8_t pull8(CPU *c, uint16_t *sp) { return rd(c, (*sp)++); }

static uint16_t pull16(CPU *c, uint16_t *sp) {
  uint8_t hi = pull8(c, sp);
  return (uint16_t)(hi << 8 | pull8(c, sp));
}

static void illegal(CPU *c) {
  fprintf(stderr, "m6809-sim: undefined instruction at %04X\n", c->op_pc);
  c->stop = STOP_ILLEGAL;
}

// ------------------------------------------------------------------- flags

static void nz8(CPU *c, uint8_t r) {
  c->cc &= (uint8_t)~(CC_N | CC_Z);
  if (r & 0x80) c->cc |= CC_N;
  if (!r) c->cc |= CC_Z;
}

static void nz16(CPU *c, uint16_t r) {
  c->cc &= (uint8_t)~(CC_N | CC_Z);
  if (r & 0x8000) c->cc |= CC_N;
  if (!r) c->cc |= CC_Z;
}

// Loads, stores and logic ops: N, Z from the value, V cleared.
static uint8_t logic8(CPU *c, uint8_t r) {
  nz8(c, r);
  c->cc &= (uint8_t)~CC_V;
  return r;
}

static uint16_t logic16(CPU *c, uint16_t r) {
  nz16(c, r);
  c->cc &= (uint8_t)~CC_V;
  return r;
}

static uint8_t add8(CPU *c, uint8_t a, uint8_t b, int carry) {
  unsigned r = (unsigned)a + b + (unsigned)carry;
  c->cc &= (uint8_t)~(CC_H | CC_N | CC_Z | CC_V | CC_C);
  if (((a & 0xF) + (b & 0xF) + (unsigned)carry) & 0x10) c->cc |= CC_H;
  if (r & 0x100) c->cc |= CC_C;
  if (~(a ^ b) & (a ^ r) & 0x80) c->cc |= CC_V;
  nz8(c, (uint8_t)r);
  return (uint8_t)r;
}

static uint8_t sub8(CPU *c, uint8_t a, uint8_t b, int borrow) {
  unsigned r = (unsigned)a - b - (unsigned)borrow;
  c->cc &= (uint8_t)~(CC_N | CC_Z | CC_V | CC_C);
  if (r & 0x100) c->cc |= CC_C;
  if ((a ^ b) & (a ^ r) & 0x80) c->cc |= CC_V;
  nz8(c, (uint8_t)r);
  return (uint8_t)r;
}

static uint16_t add16(CPU *c, uint16_t a, uint16_t b) {
  uint32_t r = (uint32_t)a + b;
  c->cc &= (uint8_t)~(CC_N | CC_Z | CC_V | CC_C);
  if (r & 0x10000) c->cc |= CC_C;
  if (~(a ^ b) & (a ^ r) & 0x8000) c->cc |= CC_V;
  nz16(c, (uint16_t)r);
  return (uint16_t)r;
}

static uint16_t sub16(CPU *c, uint16_t a, uint16_t b) {
  uint32_t r = (uint32_t)a - b;
  c->cc &= (uint8_t)~(CC_N | CC_Z | CC_V | CC_C);
  if (r & 0x10000) c->cc |= CC_C;
  if ((a ^ b) & (a ^ r) & 0x8000) c->cc |= CC_V;
  nz16(c, (uint16_t)r);
  return (uint16_t)r;
}

// Read-modify-write unary ops shared by the inherent, direct, indexed and
// extended forms. Op is the low nibble of the opcode.
static int rmw(CPU *c, int op, uint8_t *v) {
  uint8_t m = *v, r;
  switch (op) {
  case 0x0: // NEG
    r = sub8(c, 0, m, 0);
    break;
  case 0x3: // COM
    r = (uint8_t)~m;
    logic8(c, r);
    c->cc |= CC_C;
    break;
  case 0x4: // LSR
    r = m >> 1;
    c->cc = (uint8_t)((c->cc & ~(CC_C | CC_N | CC_Z)) | (m & 1));
    if (!r) c->cc |= CC_Z;
    break;
  case 0x6: // ROR
    r = (uint8_t)((m >> 1) | ((c->cc & CC_C) << 7));
    c->cc = (uint8_t)((c->cc & ~CC_C) | (m & 1));
    nz8(c, r);
    break;
  case 0x7: // ASR
    r = (uint8_t)((m >> 1) | (m & 0x80));
    c->cc = (uint8_t)((c->cc & ~CC_C) | (m & 1));
    nz8(c, r);
    break;
  case 0x8: // ASL/LSL
    r = (uint8_t)(m << 1);
    c->cc &= (uint8_t)~(CC_C | CC_V);
    if (m & 0x80) c->cc |= CC_C;
    if ((m ^ (m << 1)) & 0x80) c->cc |= CC_V;
    nz8(c, r);
    break;
  case 0x9: // ROL
    r = (uint8_t)((m << 1) | (c->cc & CC_C));
    c->cc &= (uint8_t)~(CC_C | CC_V);
    if (m & 0x80) c->cc |= CC_C;
    if ((m ^ (m << 1)) & 0x80) c->cc |= CC_V;
    nz8(c, r);
    break;
  case 0xA: // DEC
    r = (uint8_t)(m - 1);
    c->cc &= (uint8_t)~CC_V;
    if (m == 0x80) c->cc |= CC_V;
    nz8(c, r);
    break;
  case 0xC: // INC
    r = (uint8_t)(m + 1);
    c->cc &= (uint8_t)~CC_V;
    if (m == 0x7F) c->cc |= CC_V;
    nz8(c, r);
    break;
  case 0xD: // TST
    logic8(c, m);
    return 0; // no write back
  case 0xF: // CLR
    r = 0;
    c->cc = (uint8_t)((c->cc & ~(CC_N | CC_V | CC_C)) | CC_Z);
    break;
  default:
    illegal(c);
    return 0;
  }
  *v = r;
  return 1;
}

// ------------------------------------------------------------- addressing

static uint16_t *idxreg(CPU *c, int post) {
  switch ((post >> 5) & 3) {
  case 0: return &c->x;
  case 1: return &c->y;
  case 2: return &c->u;
  default: return &c->s;
  }
}

// Indexed effective address. Adds the mode's extra cycles.
static uint16_t ea_indexed(CPU *c) {
  uint8_t post = fetch(c);
  uint16_t *r = idxreg(c, post);
  if (!(post & 0x80)) {
    int off = post & 0x1F;
    if (off & 0x10) off -= 32;
    c->cycles += 1;
    return (uint16_t)(*r + off);
  }
  int indirect = post & 0x10;
  uint16_t ea;
  switch (post & 0x0F) {
  case 0x0: // ,R+
    if (indirect) goto bad;
    ea = (*r)++;
    c->cycles += 2;
    return ea;
  case 0x1: // ,R++
    ea = *r;
    *r += 2;
    c->cycles += 3;
    break;
  case 0x2: // ,-R
    if (indirect) goto bad;
    ea = --*r;
    c->cycles += 2;
    return ea;
  case 0x3: // ,--R
    *r -= 2;
    ea = *r;
    c->cycles += 3;
    break;
  case 0x4: ea = *r; break;
  case 0x5: ea = (uint16_t)(*r + (int8_t)c->b); c->cycles += 1; break;
  case 0x6: ea = (uint16_t)(*r + (int8_t)c->a); c->cycles += 1; break;
  case 0x8: ea = (uint16_t)(*r + (int8_t)fetch(c)); c->cycles += 1; break;
  case 0x9: ea = (uint16_t)(*r + fetch16(c)); c->cycles += 4; break;
  case 0xB: ea = (uint16_t)(*r + getd(c)); c->cycles += 4; break;
  case 0xC: {
    int8_t off = (int8_t)fetch(c);
    ea = (uint16_t)(c->pc + off);
    c->cycles += 1;
    break;
  }
  case 0xD: {
    uint16_t off = fetch16(c);
    ea = (uint16_t)(c->pc + off);
    c->cycles += 5;
    break;
  }
  case 0xF: // [n16]
    if (!indirect || (post & 0x60)) goto bad;
    ea = fetch16(c);
    c->cycles += 2;
    break;
  default:
    goto bad;
  }
  if (indirect) {
    ea = rd16(c, ea);
    c->cycles += 3;
  }
  return ea;
bad:
  fprintf(stderr, "m6809-sim: invalid indexed postbyte %02X at %04X\n", post,
          c->op_pc);
  c->stop = STOP_ILLEGAL;
  return 0;
}

// Effective address for the memory column of an opcode: bits 4-5 of the
// opcode select direct (01), indexed (10) or extended (11).
static uint16_t ea_mode(CPU *c, int mode) {
  switch (mode) {
  case 1: return (uint16_t)(c->dp << 8 | fetch(c));
  case 2: return ea_indexed(c);
  default: return fetch16(c);
  }
}

// --------------------------------------------------------------- branches

static int cond(CPU *c, int n) {
  int C = c->cc & CC_C, V = !!(c->cc & CC_V), Z = c->cc & CC_Z,
      N = !!(c->cc & CC_N);
  int r;
  switch (n >> 1) {
  case 0: r = 1; break;            // BRA / BRN
  case 1: r = !(C || Z); break;    // BHI / BLS
  case 2: r = !C; break;           // BHS / BLO
  case 3: r = !Z; break;           // BNE / BEQ
  case 4: r = !V; break;           // BVC / BVS
  case 5: r = !N; break;           // BPL / BMI
  case 6: r = N == V; break;       // BGE / BLT
  default: r = !Z && N == V; break; // BGT / BLE
  }
  return (n & 1) ? !r : r;
}

// ---------------------------------------------------------- register moves

// Register for a TFR/EXG nibble; 16-bit codes 0-5, 8-bit codes 8-11.
static int is16(int code) { return code < 8; }

static uint16_t getreg(CPU *c, int code) {
  switch (code) {
  case 0: return getd(c);
  case 1: return c->x;
  case 2: return c->y;
  case 3: return c->u;
  case 4: return c->s;
  case 5: return c->pc;
  case 8: return c->a;
  case 9: return c->b;
  case 10: return c->cc;
  case 11: return c->dp;
  default: return 0xFFFF;
  }
}

static void setreg(CPU *c, int code, uint16_t v) {
  switch (code) {
  case 0: setd(c, v); break;
  case 1: c->x = v; break;
  case 2: c->y = v; break;
  case 3: c->u = v; break;
  case 4: c->s = v; break;
  case 5: c->pc = v; break;
  case 8: c->a = (uint8_t)v; break;
  case 9: c->b = (uint8_t)v; break;
  case 10: c->cc = (uint8_t)v; break;
  case 11: c->dp = (uint8_t)v; break;
  default: break;
  }
}

static void tfr_exg(CPU *c, int exg) {
  uint8_t post = fetch(c);
  int src = post >> 4, dst = post & 0xF;
  if (is16(src) != is16(dst) || (src > 5 && src < 8) || src > 11 ||
      (dst > 5 && dst < 8) || dst > 11) {
    fprintf(stderr, "m6809-sim: invalid %s postbyte %02X at %04X\n",
            exg ? "EXG" : "TFR", post, c->op_pc);
    c->stop = STOP_ILLEGAL;
    return;
  }
  uint16_t s = getreg(c, src);
  if (exg)
    setreg(c, src, getreg(c, dst));
  setreg(c, dst, s);
}

// PSHS/PSHU/PULS/PULU. "Other" is U for the S forms and S for the U forms.
// Each byte moved costs a cycle; SWI and RTI include theirs in the base.
static void push_regs(CPU *c, uint16_t *sp, uint16_t other, uint8_t m) {
  if (m & 0x80) { push16(c, sp, c->pc); c->cycles += 2; }
  if (m & 0x40) { push16(c, sp, other); c->cycles += 2; }
  if (m & 0x20) { push16(c, sp, c->y); c->cycles += 2; }
  if (m & 0x10) { push16(c, sp, c->x); c->cycles += 2; }
  if (m & 0x08) { push8(c, sp, c->dp); c->cycles += 1; }
  if (m & 0x04) { push8(c, sp, c->b); c->cycles += 1; }
  if (m & 0x02) { push8(c, sp, c->a); c->cycles += 1; }
  if (m & 0x01) { push8(c, sp, c->cc); c->cycles += 1; }
}

static void pull_regs(CPU *c, uint16_t *sp, uint16_t *other, uint8_t m) {
  if (m & 0x01) { c->cc = pull8(c, sp); c->cycles += 1; }
  if (m & 0x02) { c->a = pull8(c, sp); c->cycles += 1; }
  if (m & 0x04) { c->b = pull8(c, sp); c->cycles += 1; }
  if (m & 0x08) { c->dp = pull8(c, sp); c->cycles += 1; }
  if (m & 0x10) { c->x = pull16(c, sp); c->cycles += 2; }
  if (m & 0x20) { c->y = pull16(c, sp); c->cycles += 2; }
  if (m & 0x40) { *other = pull16(c, sp); c->cycles += 2; }
  if (m & 0x80) { c->pc = pull16(c, sp); c->cycles += 2; }
}

static void swi(CPU *c, uint16_t vector, int mask_fi) {
  uint64_t cycles = c->cycles;
  c->cc |= CC_E;
  push_regs(c, &c->s, c->u, 0xFF);
  c->cycles = cycles;
  if (mask_fi)
    c->cc |= CC_I | CC_F;
  c->pc = rd16(c, vector);
}

// ------------------------------------------------------------ instructions

// Base cycles, page 0 (0 = undefined).
static const uint8_t CYCLES[256] = {
    6, 0, 0, 6, 6, 0, 6, 6, 6, 6, 6, 0, 6, 6, 3, 6,
    0, 0, 2, 4, 0, 0, 5, 9, 0, 2, 3, 0, 3, 2, 8, 6,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 5, 5, 5, 5, 0, 5, 3, 6, 20, 11, 0, 19,
    2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 0, 2,
    2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 0, 2,
    6, 0, 0, 6, 6, 0, 6, 6, 6, 6, 6, 0, 6, 6, 3, 6,
    7, 0, 0, 7, 7, 0, 7, 7, 7, 7, 7, 0, 7, 7, 4, 7,
    2, 2, 2, 4, 2, 2, 2, 0, 2, 2, 2, 2, 4, 7, 3, 0,
    4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 6, 7, 5, 5,
    4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 6, 7, 5, 5,
    5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 5, 7, 8, 6, 6,
    2, 2, 2, 4, 2, 2, 2, 0, 2, 2, 2, 2, 3, 0, 3, 0,
    4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
    4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
    5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6,
};

// The 8-bit accumulator ops in columns 0-B of rows 8-F.
static void acc_op(CPU *c, uint8_t *r, int op, uint8_t m) {
  switch (op) {
  case 0x0: *r = sub8(c, *r, m, 0); break;                  // SUB
  case 0x1: sub8(c, *r, m, 0); break;                       // CMP
  case 0x2: *r = sub8(c, *r, m, c->cc & CC_C); break;       // SBC
  case 0x4: *r = logic8(c, *r & m); break;                  // AND
  case 0x5: logic8(c, *r & m); break;                       // BIT
  case 0x6: *r = logic8(c, m); break;                       // LD
  case 0x8: *r = logic8(c, *r ^ m); break;                  // EOR
  case 0x9: *r = add8(c, *r, m, c->cc & CC_C); break;       // ADC
  case 0xA: *r = logic8(c, *r | m); break;                  // OR
  case 0xB: *r = add8(c, *r, m, 0); break;                  // ADD
  default: illegal(c); break;
  }
}

// Page 0 opcodes 0x80-0xFF.
static void exec_acc(CPU *c, uint8_t op) {
  int mode = (op >> 4) & 3; // 0 immediate, 1 direct, 2 indexed, 3 extended
  int col = op & 0xF;
  int isb = op >= 0xC0;
  uint8_t *r = isb ? &c->b : &c->a;

  if (col <= 0xB && col != 0x3 && col != 0x7) {
    uint8_t m = mode == 0 ? fetch(c) : rd(c, ea_mode(c, mode));
    acc_op(c, r, col, m);
    return;
  }
  if (col == 0x7) { // STA / STB
    if (mode == 0) { illegal(c); return; }
    uint16_t ea = ea_mode(c, mode);
    wr(c, ea, logic8(c, *r));
    return;
  }

  // 16-bit columns 3, C, D, E, F.
  uint16_t ea = 0, m = 0;
  int needs_mem = !(col == 0xD && !isb) || mode != 0; // BSR has an offset
  if (col == 0xD && !isb) {                            // BSR / JSR
    if (mode == 0) {
      int8_t off = (int8_t)fetch(c);
      push16(c, &c->s, c->pc);
      c->pc = (uint16_t)(c->pc + off);
    } else {
      ea = ea_mode(c, mode);
      push16(c, &c->s, c->pc);
      c->pc = ea;
    }
    return;
  }
  (void)needs_mem;
  int store = (col == 0xD && isb) || (col == 0xF);
  if (store) {
    if (mode == 0) { illegal(c); return; }
    ea = ea_mode(c, mode);
    uint16_t v = col == 0xD ? getd(c) : isb ? c->u : c->x;
    wr16(c, ea, logic16(c, v));
    return;
  }
  if (mode == 0) {
    m = fetch16(c);
  } else {
    ea = ea_mode(c, mode);
    m = rd16(c, ea);
  }
  switch (col) {
  case 0x3:
    setd(c, isb ? add16(c, getd(c), m) : sub16(c, getd(c), m)); // ADDD/SUBD
    break;
  case 0xC:
    if (isb) setd(c, logic16(c, m)); // LDD
    else sub16(c, c->x, m);          // CMPX
    break;
  case 0xE:
    if (isb) c->u = logic16(c, m); // LDU
    else c->x = logic16(c, m);     // LDX
    break;
  default:
    illegal(c);
    break;
  }
}

// Page 2 (0x10 prefix) and page 3 (0x11 prefix).
static void exec_page23(CPU *c, int page) {
  uint8_t op = fetch(c);
  c->cycles += 1;
  if (page == 2 && op >= 0x21 && op <= 0x2F) { // long branches: 5, 6 taken
    uint16_t off = fetch16(c);
    c->cycles += 4;
    if (cond(c, op & 0xF)) {
      c->pc = (uint16_t)(c->pc + off);
      c->cycles += 1;
    }
    return;
  }
  if (op == 0x3F) { // SWI2 / SWI3
    c->cycles += 19;
    swi(c, page == 2 ? 0xFFF4 : 0xFFF2, 0);
    return;
  }
  int mode = (op >> 4) & 3;
  int col = op & 0xF;
  if (op < 0x80 || (op >= 0xC0 && page == 3)) { illegal(c); return; }
  c->cycles += CYCLES[op];

  if (page == 3) { // CMPU, CMPS
    uint16_t m;
    if (col != 0x3 && col != 0xC) { illegal(c); return; }
    m = mode == 0 ? fetch16(c) : rd16(c, ea_mode(c, mode));
    sub16(c, col == 0x3 ? c->u : c->s, m);
    return;
  }

  int isb = op >= 0xC0;
  if (col == 0x3 && !isb) { // CMPD
    uint16_t m = mode == 0 ? fetch16(c) : rd16(c, ea_mode(c, mode));
    sub16(c, getd(c), m);
    return;
  }
  if (col == 0xC && !isb) { // CMPY
    uint16_t m = mode == 0 ? fetch16(c) : rd16(c, ea_mode(c, mode));
    sub16(c, c->y, m);
    return;
  }
  if (col == 0xE) { // LDY / LDS
    uint16_t m = mode == 0 ? fetch16(c) : rd16(c, ea_mode(c, mode));
    if (isb) c->s = logic16(c, m);
    else c->y = logic16(c, m);
    return;
  }
  if (col == 0xF && mode != 0) { // STY / STS
    uint16_t ea = ea_mode(c, mode);
    wr16(c, ea, logic16(c, isb ? c->s : c->y));
    return;
  }
  illegal(c);
}

static void step(CPU *c) {
  c->op_pc = c->pc;
  uint8_t op = fetch(c);
  c->insns++;

  if (c->trace)
    fprintf(stderr, "%04X %02X  A=%02X B=%02X X=%04X Y=%04X U=%04X S=%04X "
            "CC=%02X\n", c->op_pc, op, c->a, c->b, c->x, c->y, c->u, c->s,
            c->cc);

  if (op == 0x10 || op == 0x11) {
    exec_page23(c, op == 0x10 ? 2 : 3);
    return;
  }
  if (!CYCLES[op]) {
    illegal(c);
    return;
  }
  c->cycles += CYCLES[op];

  if (op >= 0x80) {
    exec_acc(c, op);
    return;
  }

  // Unary ops: 0x00 direct, 0x40 A, 0x50 B, 0x60 indexed, 0x70 extended.
  int row = op >> 4, col = op & 0xF;
  if (row == 0x4 || row == 0x5) {
    rmw(c, col, row == 0x4 ? &c->a : &c->b);
    return;
  }
  if (row == 0x0 || row == 0x6 || row == 0x7) {
    int mode = row == 0x0 ? 1 : row == 0x6 ? 2 : 3;
    uint16_t ea = ea_mode(c, mode);
    if (col == 0xE) { // JMP
      c->pc = ea;
      return;
    }
    uint8_t v = rd(c, ea);
    if (rmw(c, col, &v))
      wr(c, ea, v);
    return;
  }

  if (row == 0x2) { // short branches
    int8_t off = (int8_t)fetch(c);
    if (cond(c, col))
      c->pc = (uint16_t)(c->pc + off);
    return;
  }

  switch (op) {
  case 0x12: // NOP
    break;
  case 0x13: // SYNC: no interrupts in this simulator, so it would hang.
    fprintf(stderr, "m6809-sim: SYNC at %04X\n", c->op_pc);
    c->stop = STOP_ILLEGAL;
    break;
  case 0x16: { // LBRA
    uint16_t off = fetch16(c);
    c->pc = (uint16_t)(c->pc + off);
    break;
  }
  case 0x17: { // LBSR
    uint16_t off = fetch16(c);
    push16(c, &c->s, c->pc);
    c->pc = (uint16_t)(c->pc + off);
    break;
  }
  case 0x19: { // DAA
    unsigned a = c->a, adj = 0;
    unsigned lo = a & 0xF, hi = a >> 4;
    if ((c->cc & CC_H) || lo > 9) adj |= 0x06;
    if ((c->cc & CC_C) || hi > 9 || (hi > 8 && lo > 9)) adj |= 0x60;
    unsigned r = a + adj;
    c->a = (uint8_t)r;
    if (r & 0x100) c->cc |= CC_C;
    c->cc &= (uint8_t)~CC_V;
    nz8(c, c->a);
    break;
  }
  case 0x1A: c->cc |= fetch(c); break;  // ORCC
  case 0x1C: c->cc &= fetch(c); break;  // ANDCC
  case 0x1D:                            // SEX: N and Z only
    c->a = (c->b & 0x80) ? 0xFF : 0;
    nz16(c, getd(c));
    break;
  case 0x1E: tfr_exg(c, 1); break;
  case 0x1F: tfr_exg(c, 0); break;
  case 0x30: case 0x31: { // LEAX, LEAY: set Z
    uint16_t ea = ea_indexed(c);
    if (op == 0x30) c->x = ea;
    else c->y = ea;
    c->cc = (uint8_t)((c->cc & ~CC_Z) | (ea ? 0 : CC_Z));
    break;
  }
  case 0x32: c->s = ea_indexed(c); break; // LEAS
  case 0x33: c->u = ea_indexed(c); break; // LEAU
  case 0x34: push_regs(c, &c->s, c->u, fetch(c)); break;
  case 0x35: pull_regs(c, &c->s, &c->u, fetch(c)); break;
  case 0x36: push_regs(c, &c->u, c->s, fetch(c)); break;
  case 0x37: pull_regs(c, &c->u, &c->s, fetch(c)); break;
  case 0x39: c->pc = pull16(c, &c->s); break; // RTS
  case 0x3A: c->x = (uint16_t)(c->x + c->b); break; // ABX
  case 0x3B: // RTI
    c->cc = pull8(c, &c->s);
    if (c->cc & CC_E) {
      uint64_t cycles = c->cycles;
      pull_regs(c, &c->s, &c->u, 0xFE);
      c->cycles = cycles + 9;
    } else {
      c->pc = pull16(c, &c->s);
    }
    break;
  case 0x3C: // CWAI: no interrupts, so it would hang.
    fprintf(stderr, "m6809-sim: CWAI at %04X\n", c->op_pc);
    c->stop = STOP_ILLEGAL;
    break;
  case 0x3D: { // MUL
    uint16_t r = (uint16_t)(c->a * c->b);
    setd(c, r);
    c->cc &= (uint8_t)~(CC_Z | CC_C);
    if (!r) c->cc |= CC_Z;
    if (r & 0x80) c->cc |= CC_C;
    break;
  }
  case 0x3F: // SWI: the test runtime's abort().
    if (c->noports)
      swi(c, 0xFFFA, 1);
    else
      c->stop = STOP_ABORT;
    break;
  default:
    illegal(c);
    break;
  }
}

// ---------------------------------------------------------------- loading

static uint32_t be32(const uint8_t *p) {
  return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 |
         p[3];
}

static uint16_t be16(const uint8_t *p) { return (uint16_t)(p[0] << 8 | p[1]); }

static uint8_t *read_file(const char *path, size_t *len) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    perror(path);
    exit(2);
  }
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *buf = malloc((size_t)n + 1);
  if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) {
    fprintf(stderr, "m6809-sim: cannot read %s\n", path);
    exit(2);
  }
  fclose(f);
  *len = (size_t)n;
  return buf;
}

// Loads the PT_LOAD segments of a 32-bit big-endian ELF file. Returns the
// entry point.
static uint16_t load_elf(CPU *c, const uint8_t *e, size_t len) {
  if (len < 52 || e[4] != 1 || e[5] != 2) {
    fprintf(stderr, "m6809-sim: not a 32-bit big-endian ELF file\n");
    exit(2);
  }
  uint32_t phoff = be32(e + 28);
  uint16_t phentsize = be16(e + 42), phnum = be16(e + 44);
  for (unsigned i = 0; i < phnum; i++) {
    const uint8_t *ph = e + phoff + (size_t)i * phentsize;
    if (ph + 32 > e + len) break;
    if (be32(ph) != 1) continue; // PT_LOAD
    uint32_t off = be32(ph + 4), paddr = be32(ph + 12),
             filesz = be32(ph + 16), memsz = be32(ph + 20);
    if (paddr + memsz > 0x10000 || off + filesz > len) {
      fprintf(stderr, "m6809-sim: segment outside 64K\n");
      exit(2);
    }
    memcpy(c->mem + paddr, e + off, filesz);
    memset(c->mem + paddr + filesz, 0, memsz - filesz);
  }
  return (uint16_t)be32(e + 24);
}

static void usage(void) {
  fprintf(stderr,
          "usage: m6809-sim [options] program\n"
          "  program is an ELF executable, or raw with -load\n"
          "  -load ADDR    load a raw binary at ADDR (hex)\n"
          "  -entry ADDR   start at ADDR (hex) instead of the entry point\n"
          "  -max N        stop after N instructions (default 100000000)\n"
          "  -trace        print each instruction on stderr\n"
          "  -stats        print instruction and cycle counts on stderr\n"
          "  -noports      plain machine: no test ports, SWI is an interrupt\n"
          "  -dump         print the final registers and a memory checksum\n");
  exit(2);
}

int main(int argc, char **argv) {
  static CPU cpu;
  CPU *c = &cpu;
  const char *path = NULL;
  long load = -1, entry = -1;
  uint64_t max = 100000000;
  int stats = 0, dump = 0;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-load") && i + 1 < argc)
      load = strtol(argv[++i], NULL, 16);
    else if (!strcmp(argv[i], "-entry") && i + 1 < argc)
      entry = strtol(argv[++i], NULL, 16);
    else if (!strcmp(argv[i], "-max") && i + 1 < argc)
      max = strtoull(argv[++i], NULL, 10);
    else if (!strcmp(argv[i], "-trace"))
      c->trace = 1;
    else if (!strcmp(argv[i], "-stats"))
      stats = 1;
    else if (!strcmp(argv[i], "-noports"))
      c->noports = 1;
    else if (!strcmp(argv[i], "-dump"))
      dump = 1;
    else if (argv[i][0] == '-' || path)
      usage();
    else
      path = argv[i];
  }
  if (!path)
    usage();

  size_t len;
  uint8_t *data = read_file(path, &len);
  uint16_t start;
  if (load >= 0) {
    if ((size_t)load + len > 0x10000) {
      fprintf(stderr, "m6809-sim: image does not fit\n");
      return 2;
    }
    memcpy(c->mem + load, data, len);
    start = (uint16_t)load;
  } else {
    start = load_elf(c, data, len);
  }
  free(data);

  c->pc = entry >= 0 ? (uint16_t)entry : start;
  c->cc = CC_I | CC_F;
  c->s = 0xFF00; // below the ports, until the program sets its own
  c->stop = STOP_NONE;
  while (c->stop == STOP_NONE) {
    if (c->insns >= max) {
      if (!dump)
        fprintf(stderr, "m6809-sim: instruction limit reached at %04X\n",
                c->pc);
      c->stop = STOP_TIMEOUT;
      break;
    }
    step(c);
  }
  if (dump) {
    // FNV-1a over all of memory.
    uint32_t h = 2166136261u;
    for (unsigned i = 0; i < 0x10000; i++)
      h = (h ^ c->mem[i]) * 16777619u;
    printf("A=%02X B=%02X DP=%02X CC=%02X X=%04X Y=%04X U=%04X S=%04X "
           "PC=%04X MEM=%08X\n", c->a, c->b, c->dp, c->cc, c->x, c->y, c->u,
           c->s, c->pc, h);
  }
  fflush(stdout);
  if (stats)
    fprintf(stderr, "m6809-sim: %llu instructions, %llu cycles\n",
            (unsigned long long)c->insns, (unsigned long long)c->cycles);
  return c->stop;
}
