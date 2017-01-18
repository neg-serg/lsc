typedef struct buf {
	const char *buf;
	size_t len;
} buf;

buf buf_new(const char *s, size_t len);
buf slice(buf b, size_t from, size_t to);
int buf_eq(buf a, buf b);
