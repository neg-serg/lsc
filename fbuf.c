#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "slice.h"
#include "fbuf.h"
#include "util.h"

void fb_init(fb *f, int fd, size_t buflen)
{
	f->fd = fd;
	f->start = xmalloc(buflen);
	f->end = f->start+buflen-1;
	f->cursor = f->start;
	f->len = buflen;
}

void fb_drop(fb *f)
{
	free(f->start);
}

void fb_write(fb *f, const char *b, size_t len)
{
	if (unlikely(len > (size_t)(f->end-f->cursor))) {
		fb_flush(f);
		do {
			ssize_t w = write(f->fd, b, len);
			assertx(w != -1); // todo: handle properly
			len -= w;
			b += w;
		} while (len > (size_t)(f->end-f->cursor));
	}
	f->cursor = mempcpy(f->cursor, b, len);
}

void fb_write_buf(fb *f, buf b)
{
	fb_write(f, b.buf, b.len);
}

void fb_puts(fb *f, const char *b)
{
	fb_write(f, b, strlen(b));
}

void fb_putc(fb *fb, char c) {
	if (fb->cursor == fb->end) { fb_flush(fb); }
	*fb->cursor++ = c;
}

void fb_flush(fb *f)
{
	ssize_t l = f->cursor-f->start;
	ssize_t c = 0;
	do {
		ssize_t w = write(f->fd, f->start+c, l-c);
		assertx(w != -1);
		c += w;
	} while (l-c > 0);
	f->cursor = f->start;
}

static size_t fb_space(fb *f)
{
	return (size_t)(f->end-f->cursor);
}

void fb_u(fb *f, uint32_t x, int pad, char c)
{
	size_t len = 32;
	if (fb_space(f) < len) {
		fb_flush(f);
		assertx(fb_space(f) < len);
	}

	char *b = f->cursor;
	char *s = b+len-1;
	for (; x != 0; x/=10) {
		*--s = '0' + x%10;
	}
	pad = pad-((ssize_t)len-(s-b));
	for (; pad>=0; pad--) {
		*--s = c;
	}
	fb_write(f, s, len-1-(s-b));
}
