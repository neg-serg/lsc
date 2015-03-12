#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "util.h"

#define oom_msg "lsc: memory exhausted\n"

#define mem_overflow_msg "lsc: allocation size overflow\n"

// Try to write error to stderr without allocating
#define werr_s(s) write_error((s), sizeof(s)-1)

void write_error(const char *s, size_t len) {
	size_t c = 0;
	while (len-c > 0) {
		ssize_t n = write(STDERR_FILENO, s+c, len-c);
		if (n == -1) { abort(); }
		c += n;
	}
}

noreturn void die(const char *s) {
	werr_s("lsc: ");
	write_error(s, strlen(s));
	werr_s("\n");
	_exit(EXIT_FAILURE);
}

noreturn void die_errno(void) {
	char err[256];
	int r = strerror_r(errno, err, 256);
	if (r != 0) {
		die("strerror_r error");
	}
	die(err);
}

void *xmalloc(size_t nmemb, size_t size) {
	size_t total;
	if (unlikely(size_mul_overflow(nmemb, size, &total))) {
		die(mem_overflow_msg);
	}
	void *p = malloc(total);
	if (unlikely(p == NULL && total != 0)) {
		die(oom_msg);
	}
	return p;
}

void *xrealloc(void *p, size_t nmemb, size_t size) {
	size_t total;
	if (unlikely(size_mul_overflow(nmemb, size, &total))) {
		die(mem_overflow_msg);
	}
	p = realloc(p, total);
	if (unlikely(p == NULL && total != 0)) {
		die(oom_msg);
	}
	return p;
}
