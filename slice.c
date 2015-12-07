#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "slice.h"

buf buf_new(const char *s, size_t len)
{
	return (buf) { .buf = s, .len = len };
}

buf buf_cstr(const char *s)
{
	return (buf) { .buf = s, .len = strlen(s) };
}

buf buf_null(void)
{
	return (buf) { .buf = NULL, .len = 0 };
}

bool buf_empty(buf b) {
	return b.buf == NULL;
}

buf slice(buf b, size_t from, size_t to) {
	return buf_new(b.buf + from, to - from);
}

int buf_eq(buf a, buf b)
{
	return a.len == b.len && memcmp(a.buf, b.buf, a.len) == 0;
}
