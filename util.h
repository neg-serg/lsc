#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>

void die(void);

#ifdef program_name
static void die_errno(void) {
	perror(program_name);
	exit(EXIT_FAILURE);
}
#endif

#define logf(fmt, ...) (assertx(fprintf(stderr, fmt "\n", __VA_ARGS__) >= 0))
#define log(s) (logf("%s", s))
#ifdef program_name
#define warnf(fmt, ...) (logf("%s: " fmt, program_name, __VA_ARGS__))
#define warn(s) (warnf("%s", s))
#endif

#define assertx(expr) (likely(expr) ? (void)0 : abort())

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

static bool size_mul_overflow(size_t a, size_t b, size_t *result) {
#if __has_builtin(__builtin_umul_overflow) || __GNUC__ >= 5
#if INTPTR_MAX == INT32_MAX
    return __builtin_umul_overflow(a, b, result);
#else
    return __builtin_umull_overflow(a, b, result);
#endif
#else
    *result = a * b;
    static const size_t mul_no_overflow = 1UL << (sizeof(size_t) * 4);
    return (a >= mul_no_overflow || b >= mul_no_overflow) && a && SIZE_MAX / a < b;
#endif
}

void *xmalloc(size_t size);
void *xrealloc(void *p, size_t size);
void *xmallocr(size_t nmemb, size_t size);
void *xreallocr(void *p, size_t nmemb, size_t size);

#endif // UTIL_H
