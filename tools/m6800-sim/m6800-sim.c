// m6800-sim: a small Hitachi HD6301 simulator for running compiled tests.
//
// The HD6301 is a Motorola 6801 (itself an extended 6800) with a few more
// instructions: AIM, OIM, EIM, TIM, XGDX and SLP. This runs the CPU only,
// on a flat 64 KiB memory: the on-chip registers, RAM, timer and serial
// port are not modelled.
//
// It loads an ELF executable (or a raw binary), runs it, and stops when the
// program writes its exit code to the exit port.
//
// Test I/O ports:
//   $FF00  write: print the byte on stdout
//   $FF01  write: stop, with the byte as the exit status
//   $FF02  read:  next byte of stdin ($FF at end of input)
//
// Other stops:
//   SWI                    exit status 134 (abort)
//   undefined instruction  exit status 132 (also WAI and SLP, which would
//                          wait for an interrupt that never comes)
//   instruction limit      exit status 124
//   stack overflow         exit status 135: S went below __heap_start, the
//                          end of the program's data (ELF files only)
//
// The cycle counts are the HD6301 datasheet ones. They are used for
// comparing code, not for timing peripherals.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { CC_C = 0x01, CC_V = 0x02, CC_Z = 0x04, CC_N = 0x08,
       CC_I = 0x10, CC_H = 0x20, CC_ONES = 0xC0 };

enum { PORT_PUTCHAR = 0xFF00, PORT_EXIT = 0xFF01, PORT_GETCHAR = 0xFF02 };

enum { STOP_NONE = -1, STOP_ABORT = 134, STOP_ILLEGAL = 132,
       STOP_TIMEOUT = 124, STOP_STACK = 135 };

typedef struct {
  uint8_t a, b, cc;
  uint16_t x, s, pc;
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

// S points at the next free byte: a push stores, then decrements.
static void push8(CPU *c, uint8_t v) { wr(c, c->s--, v); }

static void push16(CPU *c, uint16_t v) {
  push8(c, (uint8_t)v);
  push8(c, (uint8_t)(v >> 8));
}

static uint8_t pull8(CPU *c) { return rd(c, ++c->s); }

static uint16_t pull16(CPU *c) {
  uint8_t hi = pull8(c);
  return (uint16_t)(hi << 8 | pull8(c));
}

static void illegal(CPU *c) {
  fprintf(stderr, "m6800-sim: undefined instruction at %04X\n", c->op_pc);
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

// Shifts and rotates set V to N xor C, after the operation.
static void shift_v(CPU *c) {
  c->cc &= (uint8_t)~CC_V;
  if (!(c->cc & CC_N) != !(c->cc & CC_C)) c->cc |= CC_V;
}

// Read-modify-write unary ops shared by the A, B, indexed and extended
// forms. Op is the low nibble of the opcode. Returns 0 if there is nothing
// to write back.
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
    c->cc = (uint8_t)((c->cc & ~CC_C) | (m & 1));
    nz8(c, r);
    shift_v(c);
    break;
  case 0x6: // ROR
    r = (uint8_t)((m >> 1) | ((c->cc & CC_C) << 7));
    c->cc = (uint8_t)((c->cc & ~CC_C) | (m & 1));
    nz8(c, r);
    shift_v(c);
    break;
  case 0x7: // ASR
    r = (uint8_t)((m >> 1) | (m & 0x80));
    c->cc = (uint8_t)((c->cc & ~CC_C) | (m & 1));
    nz8(c, r);
    shift_v(c);
    break;
  case 0x8: // ASL/LSL
  case 0x9: // ROL
    r = (uint8_t)((m << 1) | (op == 0x9 ? (c->cc & CC_C) : 0));
    c->cc = (uint8_t)((c->cc & ~CC_C) | (m >> 7));
    nz8(c, r);
    shift_v(c);
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
  case 0xD: // TST: V and C cleared
    logic8(c, m);
    c->cc &= (uint8_t)~CC_C;
    return 0;
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

// Effective address for the memory column of an opcode: bits 4-5 of the
// opcode select direct (01), indexed (10) or extended (11). Direct is page
// 0; indexed is X plus an unsigned 8-bit offset.
static uint16_t ea_mode(CPU *c, int mode) {
  switch (mode) {
  case 1: return fetch(c);
  case 2: return (uint16_t)(c->x + fetch(c));
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
  case 2: r = !C; break;           // BCC / BCS
  case 3: r = !Z; break;           // BNE / BEQ
  case 4: r = !V; break;           // BVC / BVS
  case 5: r = !N; break;           // BPL / BMI
  case 6: r = N == V; break;       // BGE / BLT
  default: r = !Z && N == V; break; // BGT / BLE
  }
  return (n & 1) ? !r : r;
}

// --------------------------------------------------------------- interrupts

// SWI and WAI stack PC, X, A, B and CC, in that order.
static void push_all(CPU *c) {
  push16(c, c->pc);
  push16(c, c->x);
  push8(c, c->a);
  push8(c, c->b);
  push8(c, c->cc);
}

// ------------------------------------------------------------ instructions

// Cycles per opcode (0 = undefined). 0x12 and 0x13 are undocumented.
static const uint8_t CYCLES[256] = {
    0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 1, 1, 2, 2, 4, 1, 0, 0, 0, 0,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    1, 1, 3, 3, 1, 1, 4, 4, 4, 5, 1, 10, 5, 7, 9, 12,
    1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1,
    6, 7, 7, 6, 6, 7, 6, 6, 6, 6, 6, 5, 6, 4, 3, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 4, 3, 5,
    2, 2, 2, 3, 2, 2, 2, 0, 2, 2, 2, 2, 3, 5, 3, 0,
    3, 3, 3, 4, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5, 4, 4,
    4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
    4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 6, 5, 5,
    2, 2, 2, 3, 2, 2, 2, 0, 2, 2, 2, 2, 3, 0, 3, 0,
    3, 3, 3, 4, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4,
    4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
    4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
};

// The 8-bit accumulator ops in columns 0-B of rows 8-F.
static void acc_op(CPU *c, uint8_t *r, int op, uint8_t m) {
  switch (op) {
  case 0x0: *r = sub8(c, *r, m, 0); break;                  // SUB
  case 0x1: sub8(c, *r, m, 0); break;                       // CMP
  case 0x2: *r = sub8(c, *r, m, c->cc & CC_C); break;       // SBC
  case 0x4: *r = logic8(c, *r & m); break;                  // AND
  case 0x5: logic8(c, *r & m); break;                       // BIT
  case 0x6: *r = logic8(c, m); break;                       // LDA
  case 0x8: *r = logic8(c, *r ^ m); break;                  // EOR
  case 0x9: *r = add8(c, *r, m, c->cc & CC_C); break;       // ADC
  case 0xA: *r = logic8(c, *r | m); break;                  // ORA
  case 0xB: *r = add8(c, *r, m, 0); break;                  // ADD
  default: illegal(c); break;
  }
}

// Opcodes 0x80-0xFF.
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
  if (col == 0x7) { // STAA / STAB
    if (mode == 0) { illegal(c); return; }
    uint16_t ea = ea_mode(c, mode);
    wr(c, ea, logic8(c, *r));
    return;
  }

  // 16-bit columns 3, C, D, E, F.
  if (col == 0xD && !isb) { // BSR / JSR
    if (mode == 0) {
      int8_t off = (int8_t)fetch(c);
      push16(c, c->pc);
      c->pc = (uint16_t)(c->pc + off);
    } else {
      uint16_t ea = ea_mode(c, mode);
      push16(c, c->pc);
      c->pc = ea;
    }
    return;
  }
  if ((col == 0xD && isb) || col == 0xF) { // STD, STS, STX
    if (mode == 0) { illegal(c); return; }
    uint16_t ea = ea_mode(c, mode);
    uint16_t v = col == 0xD ? getd(c) : isb ? c->x : c->s;
    wr16(c, ea, logic16(c, v));
    return;
  }
  uint16_t m = mode == 0 ? fetch16(c) : rd16(c, ea_mode(c, mode));
  switch (col) {
  case 0x3:
    setd(c, isb ? add16(c, getd(c), m) : sub16(c, getd(c), m)); // ADDD/SUBD
    break;
  case 0xC:
    if (isb) setd(c, logic16(c, m)); // LDD
    else sub16(c, c->x, m);          // CPX: all four flags on the HD6301
    break;
  default:
    if (isb) c->x = logic16(c, m); // LDX
    else c->s = logic16(c, m);     // LDS
    break;
  }
}

static void step(CPU *c) {
  c->op_pc = c->pc;
  uint8_t op = fetch(c);
  c->insns++;

  if (c->trace)
    fprintf(stderr, "%04X %02X  A=%02X B=%02X X=%04X S=%04X CC=%02X\n",
            c->op_pc, op, c->a, c->b, c->x, c->s, c->cc);

  if (!CYCLES[op]) {
    illegal(c);
    return;
  }
  c->cycles += CYCLES[op];

  if (op >= 0x80) {
    exec_acc(c, op);
    return;
  }

  int row = op >> 4, col = op & 0xF;
  if (row == 0x4 || row == 0x5) { // unary ops on A and B
    rmw(c, col, row == 0x4 ? &c->a : &c->b);
    return;
  }
  if (row == 0x6 || row == 0x7) {
    // AIM, OIM, EIM, TIM: immediate byte, then an indexed offset (0x6x)
    // or a direct address (0x7x).
    if (col == 0x1 || col == 0x2 || col == 0x5 || col == 0xB) {
      uint8_t imm = fetch(c);
      uint16_t ea = ea_mode(c, row == 0x6 ? 2 : 1);
      uint8_t v = rd(c, ea);
      switch (col) {
      case 0x1: wr(c, ea, logic8(c, v & imm)); break; // AIM
      case 0x2: wr(c, ea, logic8(c, v | imm)); break; // OIM
      case 0x5: wr(c, ea, logic8(c, v ^ imm)); break; // EIM
      default: logic8(c, v & imm); break;            // TIM
      }
      return;
    }
    uint16_t ea = ea_mode(c, row == 0x6 ? 2 : 3);
    if (col == 0xE) { // JMP
      c->pc = ea;
      return;
    }
    uint8_t v = rd(c, ea);
    if (rmw(c, col, &v))
      wr(c, ea, v);
    return;
  }

  if (row == 0x2) { // branches
    int8_t off = (int8_t)fetch(c);
    if (cond(c, col))
      c->pc = (uint16_t)(c->pc + off);
    return;
  }

  switch (op) {
  case 0x01: // NOP
    break;
  case 0x04: { // LSRD
    uint16_t d = getd(c), r = d >> 1;
    c->cc = (uint8_t)((c->cc & ~CC_C) | (d & 1));
    nz16(c, r);
    shift_v(c);
    setd(c, r);
    break;
  }
  case 0x05: setd(c, add16(c, getd(c), getd(c))); break; // ASLD
  case 0x06: c->cc = c->a | CC_ONES; break;               // TAP
  case 0x07: c->a = c->cc; break;                         // TPA
  case 0x08: // INX: Z only
  case 0x09: // DEX
    c->x = (uint16_t)(op == 0x08 ? c->x + 1 : c->x - 1);
    c->cc = (uint8_t)((c->cc & ~CC_Z) | (c->x ? 0 : CC_Z));
    break;
  case 0x0A: c->cc &= (uint8_t)~CC_V; break; // CLV
  case 0x0B: c->cc |= CC_V; break;           // SEV
  case 0x0C: c->cc &= (uint8_t)~CC_C; break; // CLC
  case 0x0D: c->cc |= CC_C; break;           // SEC
  case 0x0E: c->cc &= (uint8_t)~CC_I; break; // CLI
  case 0x0F: c->cc |= CC_I; break;           // SEI
  case 0x10: c->a = sub8(c, c->a, c->b, 0); break; // SBA
  case 0x11: sub8(c, c->a, c->b, 0); break;        // CBA
  case 0x16: c->b = logic8(c, c->a); break;        // TAB
  case 0x17: c->a = logic8(c, c->b); break;        // TBA
  case 0x18: { // XGDX
    uint16_t d = getd(c);
    setd(c, c->x);
    c->x = d;
    break;
  }
  case 0x19: { // DAA: C is set by a decimal carry, never cleared
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
  case 0x1A: // SLP: no interrupts in this simulator, so it would hang.
    fprintf(stderr, "m6800-sim: SLP at %04X\n", c->op_pc);
    c->stop = STOP_ILLEGAL;
    break;
  case 0x1B: c->a = add8(c, c->a, c->b, 0); break; // ABA
  case 0x30: c->x = (uint16_t)(c->s + 1); break;   // TSX
  case 0x31: c->s++; break;                        // INS
  case 0x32: c->a = pull8(c); break;               // PULA
  case 0x33: c->b = pull8(c); break;               // PULB
  case 0x34: c->s--; break;                        // DES
  case 0x35: c->s = (uint16_t)(c->x - 1); break;   // TXS
  case 0x36: push8(c, c->a); break;                // PSHA
  case 0x37: push8(c, c->b); break;                // PSHB
  case 0x38: c->x = pull16(c); break;              // PULX
  case 0x39: c->pc = pull16(c); break;             // RTS
  case 0x3A: c->x = (uint16_t)(c->x + c->b); break; // ABX
  case 0x3B: // RTI
    c->cc = pull8(c) | CC_ONES;
    c->b = pull8(c);
    c->a = pull8(c);
    c->x = pull16(c);
    c->pc = pull16(c);
    break;
  case 0x3C: push16(c, c->x); break; // PSHX
  case 0x3D: { // MUL: only C changes, to bit 7 of the result
    uint16_t r = (uint16_t)(c->a * c->b);
    setd(c, r);
    c->cc = (uint8_t)((c->cc & ~CC_C) | (r & 0x80 ? CC_C : 0));
    break;
  }
  case 0x3E: // WAI: stacks the registers, then waits for an interrupt
    push_all(c);
    fprintf(stderr, "m6800-sim: WAI at %04X\n", c->op_pc);
    c->stop = STOP_ILLEGAL;
    break;
  case 0x3F: // SWI: the test runtime's abort().
    if (c->noports) {
      push_all(c);
      c->cc |= CC_I;
      c->pc = rd16(c, 0xFFFA);
    } else {
      c->stop = STOP_ABORT;
    }
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
    fprintf(stderr, "m6800-sim: cannot read %s\n", path);
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
    fprintf(stderr, "m6800-sim: not a 32-bit big-endian ELF file\n");
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
      fprintf(stderr, "m6800-sim: segment outside 64K\n");
      exit(2);
    }
    memcpy(c->mem + paddr, e + off, filesz);
    memset(c->mem + paddr + filesz, 0, memsz - filesz);
  }
  return (uint16_t)be32(e + 24);
}

// The value of symbol NAME in the ELF symbol table, or -1.
static long elf_symbol(const uint8_t *e, size_t len, const char *name) {
  uint32_t shoff = be32(e + 32);
  uint16_t shentsize = be16(e + 46), shnum = be16(e + 48);
  if (!shoff || shoff + (size_t)shnum * shentsize > len)
    return -1;
  for (unsigned i = 0; i < shnum; i++) {
    const uint8_t *sh = e + shoff + (size_t)i * shentsize;
    if (be32(sh + 4) != 2) continue; // SHT_SYMTAB
    uint32_t off = be32(sh + 16), size = be32(sh + 20), link = be32(sh + 24);
    if (link >= shnum || off + size > len) return -1;
    const uint8_t *str = e + shoff + (size_t)link * shentsize;
    uint32_t stroff = be32(str + 16), strsize = be32(str + 20);
    if (stroff + strsize > len) return -1;
    for (uint32_t j = 0; j + 16 <= size; j += 16) {
      uint32_t n = be32(e + off + j);
      if (n < strsize &&
          !strncmp((const char *)e + stroff + n, name, strsize - n))
        return (long)be32(e + off + j + 4);
    }
  }
  return -1;
}

static void usage(void) {
  fprintf(stderr,
          "usage: m6800-sim [options] program\n"
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
  long stack_limit = -1;

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
      fprintf(stderr, "m6800-sim: image does not fit\n");
      return 2;
    }
    memcpy(c->mem + load, data, len);
    start = (uint16_t)load;
  } else {
    start = load_elf(c, data, len);
    stack_limit = elf_symbol(data, len, "__heap_start");
  }
  free(data);

  c->pc = entry >= 0 ? (uint16_t)entry : start;
  c->cc = CC_ONES | CC_I;
  // Below the ports, until the program sets its own. S points at the next
  // free byte, so the first push lands at $FEFF, as on m6809-sim.
  c->s = 0xFEFF;
  c->stop = STOP_NONE;
  while (c->stop == STOP_NONE) {
    if (c->insns >= max) {
      if (!dump)
        fprintf(stderr, "m6800-sim: instruction limit reached at %04X\n",
                c->pc);
      c->stop = STOP_TIMEOUT;
      break;
    }
    step(c);
    // The lowest stack byte in use is S + 1.
    if ((long)c->s + 1 < stack_limit && c->stop == STOP_NONE) {
      fprintf(stderr, "m6800-sim: stack overflow at %04X (S=%04X)\n",
              c->op_pc, c->s);
      c->stop = STOP_STACK;
    }
  }
  if (dump) {
    // FNV-1a over all of memory.
    uint32_t h = 2166136261u;
    for (unsigned i = 0; i < 0x10000; i++)
      h = (h ^ c->mem[i]) * 16777619u;
    printf("A=%02X B=%02X CC=%02X X=%04X S=%04X PC=%04X MEM=%08X\n", c->a,
           c->b, c->cc, c->x, c->s, c->pc, h);
  }
  fflush(stdout);
  if (stats)
    fprintf(stderr, "m6800-sim: %llu instructions, %llu cycles\n",
            (unsigned long long)c->insns, (unsigned long long)c->cycles);
  return c->stop;
}
