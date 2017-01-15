#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "util.h"

void *xmalloc(size_t size) {
	void *p = malloc(size);
	assertx(p != NULL);
	return p;
}

void *xrealloc(void *p, size_t size) {
	p = realloc(p, size);
	assertx(p != NULL);
	return p;
}

void *xmallocr(size_t nmemb, size_t size) {
	size_t total;
	assertx(!size_mul_overflow(nmemb, size, &total));
	void *p = malloc(total);
	assertx(p != NULL);
	return p;
}

void *xreallocr(void *p, size_t nmemb, size_t size) {
	size_t total;
	assertx(!size_mul_overflow(nmemb, size, &total));
	p = realloc(p, total);
	assertx(p != NULL);
	return p;
}

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

bool size_mul_overflow(size_t a, size_t b, size_t *result) {
#if __has_builtin(__builtin_umul_overflow) || __GNUC__ >= 5
#if INTPTR_MAX == INT32_MAX
    return __builtin_umul_overflow(a, b, result);
#else
    return __builtin_umull_overflow(a, b, result);
#endif
#else
    *result = a * b;
    static const size_t mul_no_overflow = 1UL << (sizeof(size_t) * 4);
    return (a >= mul_no_overflow || b >= mul_no_overflow) &&
		a && SIZE_MAX / a < b;
#endif
}
