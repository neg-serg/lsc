#define _GNU_SOURCE

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fbuf.h"
#include "util.h"

#define BUFLEN 65536

inline fb_t fb_new(int fd) {
	fb_t fb;
	fb.fd = fd;
	fb.cap = BUFLEN,
	fb.start = xmalloc(fb.cap);
	fb.end = fb.start+fb.cap-1;
	fb.cursor = fb.start;
	return fb;
}

void fb_free(fb_t *fb) { free(fb->start); }

inline void fb_flush(fb_t *restrict fb) {
	write(fb->fd, fb->start, fb->cursor-fb->start);
	fb->cursor = fb->start;
}

inline void fb_write(fb_t *restrict fb, const char *b, size_t len) {
	assert(len < fb->cap);
	if (unlikely(len > (size_t)(fb->end-fb->cursor))) {
		ssize_t w = write(fb->fd, fb->start, fb->cursor-fb->start);
		assert(w != -1 && w == fb->cursor-fb->start);
		fb->cursor = fb->start;
	}
	fb->cursor = mempcpy(fb->cursor, b, len);
}

inline void fb_u(fb_t *restrict fb, uint32_t x, int pad, char padc) {
	char b[32];
	char *s = b+sizeof(b)-1;
	for (; x != 0; x/=10) {
		*--s = '0' + x%10;
	}
	pad = pad-((ssize_t)sizeof(b)-(s-b));
	for (; pad>=0; pad--) {
		*--s = padc;
	}
	fb_write(fb, s, sizeof(b)-1-(s-b));
}

inline void fb_puts(fb_t *restrict fb, const char *b) {
	fb_write(fb, b, strlen(b));
}

inline void fb_putc(fb_t *restrict fb, char c) {
	if (unlikely(fb->end - fb->cursor <= 0)) {
		fb_flush(fb);
	}
	*fb->cursor++ = c;
}
