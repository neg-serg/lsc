typedef struct buf {
	const char *buf;
	size_t len;
} buf;

buf buf_new(const char *s, size_t len);
buf buf_cstr(const char *s);
buf buf_null(void);
buf slice(buf b, size_t from, size_t to);
bool buf_empty(buf b);
int buf_eq(buf a, buf b);

#define buf_const(s) {(s), sizeof(s)-1}
