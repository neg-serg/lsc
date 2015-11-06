#define _GNU_SOURCE

#include <assert.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

#include "util.h"
#include "fbuf.h"

fb fb_empty(void)
{
	fb fb = {0};
	fb.fd = -1;
	return fb;
}

void fb_init(fb *fb, int fd, size_t buflen)
{
	fb->fd = fd;
	fb->start = xmalloc(buflen);
	fb->end = fb->start+buflen-1;
	fb->cursor = fb->start;
	fb->len = buflen;
}

void fb_drop(fb *fb) {
	if (fb->start != NULL) {
		fb_flush(fb);
		free(fb->start);
	}
	fb->fd = -1;
	fb->end = NULL;
	fb->start = NULL;
	fb->cursor = NULL;
}

void fb_flush(fb *fb) {
	ssize_t l = fb->cursor-fb->start;
	ssize_t c = 0;
	do {
		ssize_t w = write(fb->fd, fb->start+c, l-c);
		assert(w != -1);
		c += w;
	} while (l-c > 0);
	fb->cursor = fb->start;
}

void fb_puts(fb *fb, const char *b) {
	fb_write(fb, b, strlen(b));
}

void fb_putc(fb *fb, char c) {
	if (fb->end - fb->cursor <= 0) {
		fb_flush(fb);
	}
	*fb->cursor++ = c;
}

//
// Number formatting
//

void fb_u(fb *fb, uint32_t x, int pad, char padc) {
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
