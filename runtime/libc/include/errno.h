#ifndef _ERRNO_H
#define _ERRNO_H
#ifdef __cplusplus
extern "C" {
#endif
/* One thread, so errno is a plain variable. */
extern int errno;
#define EDOM 33
#define ERANGE 34
#define EINVAL 22
#define ENOMEM 12
#define ENOENT 2
#define EIO 5
#define EBADF 9
#define EAGAIN 11
#define EBUSY 16
#define EEXIST 17
#define ENOSPC 28
#define ENOSYS 38
#define EOVERFLOW 75
#define EILSEQ 84
#ifdef __cplusplus
}
#endif
#endif
