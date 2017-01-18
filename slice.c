#include <string.h>

#include "slice.h"

buf buf_new(const char *s, size_t len)
{
	return (buf) { .buf = s, .len = len };
}

buf slice(buf b, size_t from, size_t to) {
	return buf_new(b.buf + from, to - from);
}

int buf_eq(buf a, buf b)
{
	return a.len == b.len && memcmp(a.buf, b.buf, a.len) == 0;
}
