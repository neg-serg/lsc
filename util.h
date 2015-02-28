#ifndef LSC_UTIL_H
#define LSC_UTIL_H

#include <assert.h>
#include <stdnoreturn.h>

// Die after printing message for errno
noreturn void die_errno(void);

// Die after printing message
noreturn void die(const char *s);

// Die if malloc fails
void *xmalloc(size_t size);

// Die if realloc fails
void *xrealloc(void *ptr, size_t size);

#undef assert

#define assert(expr) (likely(expr) ? (void)0 : abort())

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

#endif
