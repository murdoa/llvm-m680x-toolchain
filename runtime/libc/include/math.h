#ifndef _MATH_H
#define _MATH_H
#ifdef __cplusplus
extern "C" {
#endif

#define HUGE_VAL __builtin_huge_val()
#define HUGE_VALF __builtin_huge_valf()
#define INFINITY __builtin_inff()
#define NAN __builtin_nanf("")

#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4
#define fpclassify(x)                                                          \
  __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, x)
#define isnan(x) __builtin_isnan(x)
#define isinf(x) __builtin_isinf(x)
#define isfinite(x) __builtin_isfinite(x)
#define isnormal(x) __builtin_isnormal(x)
#define signbit(x) __builtin_signbit(x)

#define M_E 2.7182818284590452354
#define M_LN2 0.69314718055994530942
#define M_LN10 2.30258509299404568402
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_SQRT2 1.41421356237309504880

double fabs(double x);
double floor(double x);
double ceil(double x);
double trunc(double x);
double round(double x);
double fmod(double x, double y);
double frexp(double x, int *e);
double ldexp(double x, int e);
double scalbn(double x, int e);
double modf(double x, double *ip);
double copysign(double x, double y);
double sqrt(double x);
double exp(double x);
double log(double x);
double log10(double x);
double pow(double x, double y);

float fabsf(float x);
float floorf(float x);
float ceilf(float x);
float truncf(float x);
float roundf(float x);
float fmodf(float x, float y);
float frexpf(float x, int *e);
float ldexpf(float x, int e);
float scalbnf(float x, int e);
float modff(float x, float *ip);
float copysignf(float x, float y);
float sqrtf(float x);
float expf(float x);
float logf(float x);
float log10f(float x);
float powf(float x, float y);

#ifdef __cplusplus
}
#endif
#endif
