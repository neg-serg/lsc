#ifndef LSC_FILEVERCMP_H
#define LSC_FILEVERCMP_H

struct suf_indexed {
	buf b;
	ssize_t pos;
};

struct suf_indexed new_suf_indexed(const char *s);
struct suf_indexed new_suf_indexed_len(const char *s, size_t len);

int filevercmp(struct suf_indexed a, struct suf_indexed b);

#endif
