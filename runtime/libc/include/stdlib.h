#ifndef _STDLIB_H
#define _STDLIB_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 0x7FFF
typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;
_Noreturn void abort(void);
_Noreturn void exit(int status);
_Noreturn void _exit(int status);
int atexit(void (*fn)(void));
int abs(int x);
long labs(long x);
long long llabs(long long x);
div_t div(int n, int d);
ldiv_t ldiv(long n, long d);
lldiv_t lldiv(long long n, long long d);
void *malloc(size_t n);
void *calloc(size_t n, size_t size);
void *realloc(void *p, size_t n);
void free(void *p);
int rand(void);
void srand(unsigned seed);
int atoi(const char *s);
long atol(const char *s);
long strtol(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);
void qsort(void *base, size_t n, size_t size,
           int (*cmp)(const void *, const void *));
#ifdef __cplusplus
}
#endif
#endif
