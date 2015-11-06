#define _DEFAULT_SOURCE

#include <stdbool.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

void die(void) { exit(EXIT_FAILURE); }

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
