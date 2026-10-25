extern void out(unsigned char c);
extern unsigned char in(void);

enum state { IDLE, START, DATA, STOP };

static enum state st;
static unsigned char count, value;

int step(unsigned char bit) {
  switch (st) {
  case IDLE:
    if (!bit)
      st = START;
    break;
  case START:
    st = DATA;
    count = 0;
    value = 0;
    break;
  case DATA:
    value = value >> 1 | (bit ? 0x80 : 0);
    if (++count == 8)
      st = STOP;
    break;
  case STOP:
    st = IDLE;
    if (bit) {
      out(value);
      return 1;
    }
    break;
  }
  return 0;
}

unsigned char keypad(void) {
  unsigned char k = in();
  switch (k) {
  case 0x01: return '1';
  case 0x02: return '2';
  case 0x04: return '3';
  case 0x08: return '4';
  case 0x10: return '5';
  case 0x20: return '6';
  case 0x40: return '7';
  case 0x80: return '8';
  default: return 0;
  }
}

void puts_out(const char *s) {
  while (*s)
    out(*s++);
}

int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
