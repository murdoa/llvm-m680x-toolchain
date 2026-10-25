#undef assert
#ifdef NDEBUG
#define assert(e) ((void)0)
#else
#ifdef __cplusplus
extern "C"
#endif
_Noreturn void __assert_fail(const char *expr, const char *file, int line,
                             const char *func);
#define assert(e)                                                              \
  ((e) ? (void)0 : __assert_fail(#e, __FILE__, __LINE__, __func__))
#endif

#if !defined(__cplusplus) && __STDC_VERSION__ >= 201112L && !defined(static_assert)
#define static_assert _Static_assert
#endif
