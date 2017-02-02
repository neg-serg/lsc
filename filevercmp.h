struct suf_indexed {
	buf b;
	size_t pos;
};

struct suf_indexed new_suf_indexed(const char *s);
struct suf_indexed new_suf_indexed_len(const char *s, size_t len);

int filevercmp(struct suf_indexed a, struct suf_indexed b);
