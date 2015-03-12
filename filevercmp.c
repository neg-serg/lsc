#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

#include "filevercmp.h"
#include "util.h"

bool c_isdigit(char c);
bool c_isalpha(char c);
int suffix(const char *s, size_t len);
int order(char c);
int verrevcmp(const char *a, size_t alen, const char *b, size_t blen);

bool c_isdigit(char c) { return (unsigned char)(c - '0') < 10; }
bool c_isalpha(char c) { return (unsigned char)((c | 32) - 'a') < 26; }

int suffix(const char *s, size_t len) {
	bool read_alphat = false;
	size_t matched = 0;
	size_t j = 0;
	for (ssize_t i = ((ssize_t)len - 1); i >= 0; i--) {
		char c = s[i];
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
	return len - matched;
}

int order(char c) {
	if (c_isalpha(c)) {
		return (int)c;
	} else if (c_isdigit(c)) {
		return 0;
	} else if (c == '~') {
		return -1;
	}
	return (int)c + 256;
}


int verrevcmp(const char *a, size_t alen, const char *b, size_t blen) {
	size_t ai = 0, bi = 0;
	while (ai < alen || bi < blen) {
		int first_diff= 0;
		while ((ai < alen && !c_isdigit(a[ai])) ||
		       (bi < blen && !c_isdigit(b[bi]))) {
			int ac = (ai == alen) ? 0 : order (a[ai]);
			int bc = (bi == blen) ? 0 : order (b[bi]);
			if (ac != bc) {
				return ac - bc;
			}
			ai++;
			bi++;
		}
		while (ai < alen && a[ai] == '0') { ai++; }
		while (bi < blen && b[bi] == '0') { bi++; }
		while (ai < alen && c_isdigit(a[ai]) &&
		       bi < blen && c_isdigit(b[bi])) {
			if (first_diff == 0) {
				first_diff = (int)a[ai] - (int)b[bi];
			}
			ai++;
			bi++;
		}
		if (c_isdigit(a[ai])) { return  1; }
		if (c_isdigit(b[bi])) { return -1; }
		if (first_diff) { return first_diff; }
	}
	return 0;
}

struct suf_indexed new_suf_indexed(const char *s) {
	return new_suf_indexed_len(s, strlen(s));
}

struct suf_indexed new_suf_indexed_len(const char *s, size_t len) {
	if (len != 0 && s[0] == '.') {
		return (struct suf_indexed) {
			.str = s,
			.pos = suffix(s+1, len-1),
			.len = len,
		};
	} else {
		return (struct suf_indexed) {
			.str = s,
			.pos = suffix(s, len),
			.len = len,
		};
	}
}

int filevercmp(struct suf_indexed a, struct suf_indexed b) {
	int scmp = strcmp((char *)a.str, (char *)b.str);
	if (scmp == 0) { return 0; }
	if (*a.str == '\0') { return -1; }
	if (*b.str == '\0') { return  1; }

	// Special case for hidden files
	if (a.str[0] == '.') {
		if (b.str[0] == '.') {
			a.str++; a.len--;
			b.str++; b.len--;
		} else {
			return -1;
		}
	} else if (b.str[0] == '.') {
		return 1;
	}

	int result;
	if ((a.pos == b.pos) && (strncmp((char *)a.str, (char *)b.str, a.pos) == 0)) {
		result = verrevcmp(a.str+a.pos, a.len-a.pos, b.str+b.pos, b.len-b.pos);
	} else {
		result = verrevcmp(a.str, a.pos, b.str, b.pos);
	}

	return result == 0 ? scmp : result;
}
