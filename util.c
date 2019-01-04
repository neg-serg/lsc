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

#define MUL_NO_OVERFLOW ((size_t)1 << (sizeof(size_t) * 4))

bool size_mul_overflow(size_t a, size_t b, size_t *result) {
    *result = a * b;
    return (a >= MUL_NO_OVERFLOW || b >= MUL_NO_OVERFLOW) &&
		a && SIZE_MAX / a < b;
}
