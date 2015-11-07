#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>

void die(void);

void die_errno(void);

#define logf(fmt, ...) (assertx(fprintf(stderr, fmt "\n", __VA_ARGS__) >= 0))
#define log(s) (logf("%s", s))
#define warnf(fmt, ...) (logf("%s: " fmt, program_invocation_name, __VA_ARGS__))
#define warn(s) (warnf("%s", s))

#define assertx(expr) (likely(expr) ? (void)0 : abort())

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

static inline bool size_mul_overflow(size_t a, size_t b, size_t *result) {
	static_assert(INTPTR_MAX != 0, "stdint not included");
#if defined(__clang__) || __GNUC__ >= 5
#if INTPTR_MAX == INT32_MAX
	return __builtin_umul_overflow(a, b, result);
#else
	return __builtin_umull_overflow(a, b, result);
#endif
#else
	*result = a * b;
	return a && *result / a != b;
#endif
}

static inline bool size_add_overflow(size_t a, size_t b, size_t *result) {
	static_assert(INTPTR_MAX != 0, "stdint not included");
#if defined(__clang__) || __GNUC__ >= 5
#if INTPTR_MAX == INT32_MAX
	return __builtin_uadd_overflow(a, b, result);
#else
	return __builtin_uaddl_overflow(a, b, result);
#endif
#else
	*result = a + b;
	return !(a < SIZE_MAX - b);
#endif
}

void *xmalloc(size_t size);
void *xrealloc(void *p, size_t size);
void *xmallocr(size_t nmemb, size_t size);
void *xreallocr(void *p, size_t nmemb, size_t size);

#endif // UTIL_H
