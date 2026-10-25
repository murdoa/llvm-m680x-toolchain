#ifndef _STDIO_H
#define _STDIO_H
#include <stdarg.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define EOF (-1)
typedef struct __file FILE;
extern FILE *const stdin, *const stdout, *const stderr;
int putchar(int c);
int getchar(void);
int puts(const char *s);
int fputc(int c, FILE *f);
int fputs(const char *s, FILE *f);
#define putc fputc
int printf(const char *fmt, ...);
int fprintf(FILE *f, const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t n, const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int vfprintf(FILE *f, const char *fmt, va_list ap);
int vsprintf(char *buf, const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
/* There are no files: these fail and set errno to ENOSYS. */
int remove(const char *path);
int rename(const char *from, const char *to);
#ifdef __cplusplus
}
#endif
#endif
