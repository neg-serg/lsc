#include <stdbool.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

void die(void) { exit(EXIT_FAILURE); }

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

void *xmalloc(size_t size) {
	void *p = malloc(size);
	if (unlikely(p == NULL && size != 0)) {
		abort();
	}
	return p;
}

void *xrealloc(void *p, size_t size) {
	p = realloc(p, size);
	if (unlikely(p == NULL && size != 0)) {
		abort();
	}
	return p;
}

void *xmallocr(size_t nmemb, size_t size) {
	size_t total;
	if (unlikely(size_mul_overflow(nmemb, size, &total))) {
		abort();
	}
	void *p = malloc(total);
	if (unlikely(p == NULL && total != 0)) {
		abort();
	}
	return p;
}

void *xreallocr(void *p, size_t nmemb, size_t size) {
	size_t total;
	if (unlikely(size_mul_overflow(nmemb, size, &total))) {
		abort();
	}
	p = realloc(p, total);
	if (unlikely(p == NULL && total != 0)) {
		abort();
	}
	return p;
}
