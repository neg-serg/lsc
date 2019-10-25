#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ls_colors.h"
#include "util.h"

static const char *label_codes[] = {
	"lc", "rc", "ec", "rs", "no", "fi", "di", "ln",
	"pi", "so", "bd", "cd", "mi", "or", "ex", "do",
	"su", "sg", "st", "ow", "tw", "ca", "mh", "cl",
	NULL,
};

static int ext_pair_cmp(const void *va, const void *vb) {
	struct ext_pair *a = (struct ext_pair *const)va;
	struct ext_pair *b = (struct ext_pair *const)vb;
	return strcmp(a->ext, b->ext);
}

const char *lsc_lookup(struct ls_colors *lsc, const char *ext) {
	struct ext_pair k = { .ext = ext, .color = NULL }, *res;
	res = bsearch(&k, lsc->ext_map, lsc->exts, sizeof(k), ext_pair_cmp);
	return res ? res->color : NULL;
}

void lsc_parse(struct ls_colors *lsc, char *lsc_env) {
	size_t exts = 0, exti = 0;
	size_t len = strlen(lsc_env);
	for (size_t i = 0; i < len; i++) if (lsc_env[i] == '*') exts++;
	lsc->ext_map = xmallocr(exts, sizeof(struct ext_pair));
	memset(lsc->labels, 0, sizeof(lsc->labels));
	bool eq = false;
	size_t kbegin = 0, kend = 0;
	for (size_t i = 0; i < len; i++) {
		char c = lsc_env[i];
		if (c == '=') {
			kend = i;
			eq = true;
			continue;
		}
		if (!eq || c != ':')
			continue;
		lsc_env[kend] = lsc_env[i] = '\0';
		char *k = &lsc_env[kbegin];
		char *v = &lsc_env[kend + 1];
		if (*k == '*') {
			assertx(exti < exts);
			lsc->ext_map[exti++] = (struct ext_pair) { k + 1, v };
		} else if (kend - kbegin == 2) {
			size_t i = 0;
			const char **p;
			for (p = label_codes; *p; p++, i++)
				if (k[0] == (*p)[0] && k[1] == (*p)[1])
					break;
			if (*p)
				lsc->labels[i] = v;
		}
		kbegin = i + 1;
		i += 2;
		eq = false;
	}
	lsc->exts = exti;
	qsort(lsc->ext_map, lsc->exts, sizeof(*lsc->ext_map), ext_pair_cmp);
}

#ifdef TEST
#include <stdio.h>
#include <unistd.h>
int main(void) {
#define cap 64*1024
	char a[cap];
	size_t len = 0;
	for (;;) {
		ssize_t n = read(0, a+len, cap-len-1);
		if (cap-len-1 == 0) return 1;
		if (n == 0) break;
		if (n == -1) {
			perror("ls_colors");
			return 1;
		}
		len += n;
	}
	a[++len] = 0;
	struct ls_colors lsc = {0};
	lsc_parse(&lsc, a);
}
#endif
