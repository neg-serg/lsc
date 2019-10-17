#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "filevercmp.h"
#include "util.h"

static size_t suffix(const char *s, size_t len) {
	bool read_alphat = false;
	size_t matched = 0;
	size_t j = 0;
	for (ssize_t i = ((ssize_t)len - 1); i >= 0; i--) {
		char c = s[i];
		if (isalpha(c) || c == '~')
			read_alphat = true;
		else if (read_alphat && c == '.')
			matched = j + 1;
		else if (isdigit(c))
			read_alphat = false;
		else
			break;
		j++;
	}
	return len - matched;
}

static int order(char c) {
	if (isalpha(c)) return c;
	if (isdigit(c)) return 0;
	if (c == '~') return -1;
	return (int)c + 256;
}

static int verrevcmp(const char *a, const char *b, size_t al, size_t bl) {
	size_t ai = 0, bi = 0;
	while (ai < al || bi < bl) {
		int first_diff = 0;
		while ((ai < al && !isdigit(a[ai])) || (bi < bl && !isdigit(b[bi]))) {
			int ac = (ai == al) ? 0 : order(a[ai]);
			int bc = (bi == bl) ? 0 : order(b[bi]);
			if (ac != bc) return ac - bc;
			ai++; bi++;
		}
		while (a[ai] == '0') ai++;
		while (b[bi] == '0') bi++;
		while (isdigit(a[ai]) && isdigit(b[bi])) {
			if (!first_diff) first_diff = a[ai] - b[bi];
			ai++; bi++;
		}
		if (isdigit(a[ai])) return 1;
		if (isdigit(b[bi])) return -1;
		if (first_diff) return first_diff;
	}
	return 0;
}

size_t suf_index(const char *s, size_t len) {
	if (len != 0 && s[0] == '.') { s++; len--; }
	return suffix(s, len);
}

int filevercmp(const char *a, const char *b, size_t al, size_t bl, size_t ai, size_t bi) {
	int scmp = strcmp((char *)a, (char *)b);
	if (scmp == 0) return 0;
	if (al == 0) return -1;
	if (bl == 0) return 1;

	// Special cases for hidden files
	if (a[0] == '.' && b[0] != '.')
		return -1;
	if (a[0] != '.' && b[0] == '.')
		return 1;
	if (a[0] == '.' && b[0] == '.')
		a++, al--, b++, bl--;

	int result;
	if (ai == bi && strncmp((char *)a, (char *)b, ai) == 0) {
		a += ai; ai = al - ai;
		b += bi; bi = bl - bi;
	}
	result = verrevcmp(a, b, ai, bi);
	return result == 0 ? scmp : result;
}
