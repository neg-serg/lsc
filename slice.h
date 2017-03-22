typedef struct buf {
	const char *buf;
	size_t len;
} buf;

static inline buf buf_new(const char *s, size_t len) {
	return (buf) { .buf = s, .len = len };
}

static inline buf slice(buf b, size_t from, size_t to) {
	return buf_new(b.buf + from, to - from);
}

static inline int buf_eq(buf a, buf b)
{
	return a.len == b.len && memcmp(a.buf, b.buf, a.len) == 0;
}
