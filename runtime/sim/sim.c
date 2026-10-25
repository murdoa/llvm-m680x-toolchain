// Board support for m6809-sim and m6800-sim: their ports at $FF00 (write a
// character), $FF01 (exit with a status) and $FF02 (read a character, $FF at
// the end).

#define SIM_PUTC (*(volatile unsigned char *)0xFF00)
#define SIM_EXIT (*(volatile unsigned char *)0xFF01)
#define SIM_GETC (*(volatile unsigned char *)0xFF02)

void _exit(int status) {
  SIM_EXIT = (unsigned char)status;
  for (;;)
    ;
}

int __board_putc(int c) {
  SIM_PUTC = (unsigned char)c;
  return (unsigned char)c;
}

int __board_getc(void) {
  unsigned char c = SIM_GETC;
  return c == 0xFF ? -1 : c;
}
