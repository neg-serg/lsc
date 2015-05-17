#ifndef LSC_FBUF_H
#define LSC_FBUF_H

#include <assert.h>

typedef struct fb {
	char *start;
	char *end;
	char *cursor;
	size_t len;
	int fd;
} fb;

void fb_init(fb *fb, int fd, size_t buflen);
void fb_drop(fb *fb);
void fb_flush(fb *fb);
void fb_puts(fb *fb, const char *s);
void fb_putc(fb *fb, char c);
int fb_printf_hack(fb *fb, size_t maxlen, const char *fmt, ...);
void fb_u(fb *fb, uint32_t x, int pad, char padc);
int fb_fp(fb *f, long double y, int w, int p);

static inline void fb_write(fb *fb, const char *b, size_t len) {
	assert(len < fb->len);
	if (unlikely(len > (size_t)(fb->end-fb->cursor))) {
		fb_flush(fb);
	}
	fb->cursor = mempcpy(fb->cursor, b, len);
}

// write constant string
#define fb_ws(fb, s) fb_write((fb), (s), sizeof(s)-1)

// sized string
struct sstr {
	const char *s;
	size_t len;
};

// sized string from literal
#define SSTR(s) {(s), sizeof(s)-1}

#define fb_sstr(out, ts) fb_write((out), (ts).s, (ts).len)

#endif
