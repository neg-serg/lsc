#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "slice.h"
#include "filevercmp.h"
#include "util.h"

static bool c_isdigit(char c)
{
	return (unsigned char)(c - '0') < 10;
}

static bool c_isalpha(char c)
{
	return (unsigned char)((c | 32) - 'a') < 26;
}

static size_t suffix(buf s)
{
	bool read_alphat = false;
	size_t matched = 0;
	size_t j = 0;
	for (ssize_t i = ((ssize_t)s.len - 1); i >= 0; i--) {
		char c = s.buf[i];
		if (c_isalpha(c) || c == '~') {
			read_alphat = true;
		} else if (read_alphat && c == '.') {
			matched = j + 1;
		} else if (c_isdigit(c)) {
			read_alphat = false;
		} else {
			break;
		}
		j++;
	}
	return s.len - matched;
}

static int order(char c)
{
	if (c_isalpha(c)) {
		return c;
	} else if (c_isdigit(c)) {
		return 0;
	} else if (c == '~') {
		return -1;
	}
	return (int)c + 256;
}

static int verrevcmp(buf a, buf b)
{
	size_t ai = 0, bi = 0;
	while (ai < a.len || bi < b.len) {
		int first_diff = 0;
		while ((ai < a.len && !c_isdigit(a.buf[ai])) ||
		       (bi < b.len && !c_isdigit(b.buf[bi]))) {
			int ac = (ai == a.len) ? 0 : order(a.buf[ai]);
			int bc = (bi == b.len) ? 0 : order(b.buf[bi]);
			if (ac != bc)
				return ac - bc;
			ai++;
			bi++;
		}
		while (a.buf[ai] == '0') ai++;
		while (b.buf[bi] == '0') bi++;
		while (c_isdigit(a.buf[ai]) && c_isdigit(b.buf[bi])) {
			if (!first_diff)
				first_diff = a.buf[ai] - b.buf[bi];
			ai++;
			bi++;
		}
		if (c_isdigit(a.buf[ai])) { return  1; }
		if (c_isdigit(b.buf[bi])) { return -1; }
		if (first_diff) { return first_diff; }
	}
	return 0;
}

struct suf_indexed new_suf_indexed(const char *s)
{
	return new_suf_indexed_len(s, strlen(s));
}

struct suf_indexed new_suf_indexed_len(const char *s, size_t len)
{
	struct suf_indexed si;
	si.b = buf_new(s, len);
	if (len != 0 && s[0] == '.')
		si.pos = suffix(slice(si.b, 1, len));
	else
		si.pos = suffix(si.b);
	return si;
}

int filevercmp(struct suf_indexed a, struct suf_indexed b)
{
	int scmp = strcmp((char *)a.b.buf, (char *)b.b.buf);
	if (scmp == 0) { return 0; }
	if (a.b.len == 0) { return -1; }
	if (b.b.len == 0) { return  1; }

	// Special case for hidden files
	if (a.b.buf[0] == '.') {
		if (b.b.buf[0] == '.') {
			a.b = slice(a.b, 1, a.b.len);
			b.b = slice(b.b, 1, b.b.len);
		} else {
			return -1;
		}
	} else if (b.b.buf[0] == '.') {
		return 1;
	}

	buf ab, bb;
	ab = slice(a.b, 0, a.pos ? a.pos : a.b.len);
	bb = slice(b.b, 0, b.pos ? b.pos : b.b.len);
	int result;
	if ((a.pos == b.pos) &&
	    (strncmp((char *)a.b.buf, (char *)b.b.buf, a.pos) == 0)) {
		ab = slice(a.b, a.pos, a.b.len);
		bb = slice(b.b, b.pos, b.b.len);
	}
	result = verrevcmp(ab, bb);
	return result == 0 ? scmp : result;
}
