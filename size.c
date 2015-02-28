#define _GNU_SOURCE

#include <stddef.h>
#include <stdint.h>

#include "conf.h"
#include "fbuf.h"
#include "fp.h"
#include "size.h"

void write_size(fb_t *out, size_t size, const char *const sufs[7]);

inline void write_size(fb_t *out, size_t size, const char *const sufs[7]) {
	double s = (double)size;
	size_t m = 0;
	while (s >= 1000.0) {
		m++;
		s /= 1024;
	}
	if (s == 0) {
		fb_ws(out, "  0");
	} else if (s < 9.95) {
		fmt_fp(out, s, 0, 1);
		//fb_printf_hack(out, 10, "%.1f", s);
	} else {
		//fmt_fp(out, s, 3, 0);
		//fb_u(out, s, 3, ' ');
		fb_u(out, (uint32_t)(s+0.5), 3, ' ');
		//fb_printf_hack(out, 10, "%3.0f", s);
	}
	fb_puts(out, sufs[m]);
}

inline void size_color(fb_t *out, size_t size) {
	fb_ws(out, cSize);
	write_size(out, size, cSizes);
}

inline void size_no_color(fb_t *out, size_t size) {
	fb_ws(out, nSize);
	write_size(out, size, nSizes);
}
