#define _GNU_SOURCE

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "colorh.h"
#include "ht.h"
#include "type.h"
#include "util.h"
#include "xxhash/xxhash.h"

#include "lscolors.h"

bool color_sym_target = false;

// TODO: use these more
// TODO: replace with struct
static char *ls_color_types[] = {
	"\033[",  // "lc": Left of color sequence
	"m",      // "rc": Right of color sequence
	"",       // "ec": End color (replaces lc+no+rc)
	"0",      // "rs": Reset to ordinary colors
	"",       // "no": Normal
	"",       // "fi": File: default
	"01;34",  // "di": Directory: bright blue
	"01;36",  // "ln": Symlink: bright cyan
	"33",     // "pi": Pipe: yellow/brown
	"01;35",  // "so": Socket: bright magenta
	"01;33",  // "bd": Block device: bright yellow
	"01;33",  // "cd": Char device: bright yellow
	"",       // "mi": Missing file: undefined
	"",       // "or": Orphaned symlink: undefined
	"01;32",  // "ex": Executable: bright green
	"01;35",  // "do": Door: bright magenta
	"37;41",  // "su": setuid: white on red
	"30;43",  // "sg": setgid: black on yellow
	"37;44",  // "st": sticky: black on blue
	"34;42",  // "ow": other-writable: blue on green
	"30;42",  // "tw": ow w/ sticky: black on green
	"30;41",  // "ca": black on red
	"",       // "mh": disabled by default
	"\033[K", // "cl": clear to end of line
};

static char *lsc_env;
static te_t *ht;

static inline int keyeq(const te_key_t a, const te_key_t b) {
	return a.len == b.len && memcmp(a.key, b.key, a.len) == 0;
}

static inline unsigned keyhash(const te_key_t a) {
	return XXH32(a.key, a.len, 0);
}

void parse_ls_color() {
	lsc_env = getenv("LS_COLORS");
	ht = te_alloc(keyhash, keyeq);
	bool eq = false;
	size_t kb = 0, ke = 0;
	for (size_t i = 0; lsc_env[i] != '\0'; i++) {
		const char b = lsc_env[i];
		if (b == '=') {
			ke = i;
			eq = true;
		} else if (eq && b == ':') {
			if (lsc_env[kb] == '*') {
				te_key_t k;
				te_value_t v;

				lsc_env[ke] = '\0';
				k.len = ke-kb-1;
				k.key = lsc_env+kb+1;

				lsc_env[i] = '\0';
				v = lsc_env+ke+1;

				te_set(ht, k, v);
			} else {
				// TODO make sure it's 2 long
				const char k[] = {lsc_env[kb], lsc_env[kb+1]};
				const struct ind_name *out = ind_name_lookup((const char*)&k, 2);
				//XXX handle error better
				assert(out != NULL);
				lsc_env[i] = '\0';
				ls_color_types[out->code] = lsc_env+ke+1;
			}
			kb = i + 1;
			i += 2;
			eq = false;
		}
	}
	if (strcmp(ls_color_types[IND_LINK], "target") == 0) {
		color_sym_target = true;
	}
}

//te_value_t te_get(te_t *ht, te_key_t key);
const char *color(char *name, size_t len, enum indicator in) {
	if (in == IND_FILE || in == IND_LINK) {
		char *n = memrchr(name, '.', len);
		if (n != NULL) {
			te_key_t k;
			k.key = n;
			k.len = name+len-n;
			char *ret = te_get(ht, k);
			if (ret != NULL) {
				return ret;
			}
		}
	}
	if (in == IND_LINK) {
		return "38;5;8;3";
	}
	return ls_color_types[in];
}

//var lsColorSuffix map[string]string
