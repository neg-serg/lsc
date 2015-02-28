#include <sys/types.h>
#include <string.h>

#include "type.h"

#include "colorh.h"

inline const struct ind_name *ind_name_lookup(const char *str, const unsigned int len) {
	static const unsigned char asso_values[] = {
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 23, 30,
		10, 10, 13, 54, 11, 28,  3, 54, 18, 15,
		6,  5,  0, 54, 20,  0, 15,  1,  8, 54,
		10, 28, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54,
	};

	static const struct ind_name wordlist[] = {
		{"so", IND_SOCK},
		{"st", IND_STICKY},
		{NULL, 0},
		{"pi", IND_FIFO},
		{NULL, 0},
		{"or", IND_ORPHAN},
		{"no", IND_NORM},
		{NULL, 0},
		{"su", IND_SETUID},
		{NULL, 0},
		{"do", IND_DOOR},
		{"sg", IND_SETGID},
		{NULL, 0},
		{"di", IND_DIR},
		{NULL, 0},
		{"ow", IND_OTHERWRITABLE},
		{"fi", IND_FILE},
		{NULL, 0},
		{"mi", IND_MISSING},
		{NULL, 0},
		{"ec", IND_END},
		{NULL, 0}, {NULL, 0},
		{"ln", IND_LINK},
		{NULL, 0},
		{"tw", IND_STICKYOTHERWRITABLE},
		{NULL, 0}, {NULL, 0},
		{"lc", IND_LEFT},
		{NULL, 0},
		{"rc", IND_RIGHT},
		{NULL, 0}, {NULL, 0},
		{"bd", IND_BLK},
		{NULL, 0},
		{"rs", IND_RESET},
		{NULL, 0}, {NULL, 0},
		{"ex", IND_EXEC},
		{NULL, 0},
		{"cd", IND_CHR},
		{NULL, 0}, {NULL, 0},
		{"mh", IND_MULTIHARDLINK},
		{NULL, 0},
		{"cl", IND_CLRTOEOL},
		{NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0},
		{NULL, 0}, {NULL, 0}, {NULL, 0},
		{"ca", IND_CAP},
	};

	if (len == WORD_LENGTH) {
		const int key = asso_values[(unsigned char)str[1]+1] + asso_values[(unsigned char)str[0]];
		if (key <= MAX_HASH_VALUE && key >= 0) {
			const char *s = wordlist[key].name;
			// WORD_LENGTH is 2
			if (s != NULL && str[0] == s[0] && str[1] == s[1]) {
				return &wordlist[key];
			}
		}
	}
	return 0;
}
