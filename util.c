#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <unistd.h>

#include "util.h"

#define oom_msg "lsc: memory exhausted\n"

noreturn void die(const char *s) {
	fputs(s, stderr);
	exit(1);
}

noreturn void die_errno(void) {
	perror("lsc");
	exit(1);
}

inline void *xmalloc(size_t size) {
	void *p = malloc(size);
	if (unlikely(p == NULL && size != 0)) {
		write(STDERR_FILENO, oom_msg, sizeof(oom_msg) - 1);
		abort();
	}
	return p;
}

inline void *xrealloc(void *p, size_t size) {
	p = realloc(p, size);
	if (unlikely(p == NULL && size != 0)) {
		write(STDERR_FILENO, oom_msg, sizeof(oom_msg));
		abort();
	}
	return p;
}
