#define _DEFAULT_SOURCE

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

#include "fbuf.h"
#include "fp.h"
#include "time.h"

#include "conf.h"
#include "filevercmp.h"
#include "modes.h"
#include "size.h"
#include "stat.h"
#include "type.h"
#include "util.h"

#include "args.h"
#include "lscolors.h"

static inline void name(fb_t *out, const struct file_info *f) {
	enum indicator t;
	if (f->linkname.str != NULL && *f->linkname.str != '\0') {
		if (!f->linkok) {
			t = IND_ORPHAN;
		} else if (color_sym_target) {
			t = color_type(f->linkmode);
		} else {
			t = color_type(f->mode);
		}
	} else {
		t = color_type(f->mode);
	}
	const char *c = color((char*)f->name.str, f->name.len, t);
	if (c == NULL) {
		fb_puts(out, f->name.str);
	} else {
		fb_ws(out, cESC);
		fb_puts(out, c);
		fb_putc(out, 'm');
		fb_puts(out, f->name.str);
		fb_ws(out, cEnd);
	}
	if (f->linkname.str != NULL) {
		enum indicator lnt;
		if (!f->linkok) {
			lnt = IND_MISSING;
		} else {
			lnt = color_type(f->linkmode);
		}
		const char *lc = color((char*)f->linkname.str, f->linkname.len, lnt);
		if (lc == NULL || *lc == '\0') {
			fb_puts(out, f->linkname.str);
		} else {
			fb_ws(out, cSymDelim cESC);
			fb_puts(out, lc);
			fb_putc(out, 'm');
			fb_puts(out, (char *)f->linkname.str);
			fb_ws(out, cEnd);
		}
	}
	if (opts.classify) {
		switch (t) {
		case IND_DIR:
			fb_putc(out, '/');
			break;
		case IND_EXEC:
			fb_putc(out, '*');
			break;
		case IND_FIFO:
			fb_putc(out, '|');
			break;
		case IND_SOCK:
			fb_putc(out, '=');
			break;
		}
	}
}

int main(int argc, char **argv) {
	parse_args(argc, argv);
	parse_ls_color();
	//bool colorize = (opts.color == COLOR_AUTO && isatty(STDOUT_FILENO)) || opts.color == COLOR_ALWAYS;
	struct file_list l;
	file_list_init(&l);
	fb_t out = fb_new(STDOUT_FILENO);
	uint32_t now = current_time();
	for (size_t i = 0; i < opts.restc; i++) {
		ls(&l, opts.rest[i]);
		opts.sorter(l.data, l.len);
		for (size_t j = 0; j < l.len; j++) {
			strmode(&out, l.data[j].mode);
			reltime(&out, now, l.data[j].time);
			fb_ws(&out, cCol);
			size_color(&out, l.data[j].size);
			fb_ws(&out, cCol);
			name(&out, &l.data[j]);
			fb_putc(&out, '\n');
		}
		file_list_clear(&l);
	};
	fb_flush(&out);
	file_list_free(&l);
	fb_free(&out);
}
