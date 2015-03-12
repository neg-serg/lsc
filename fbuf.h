#ifndef LSC_FBUF_H
#define LSC_FBUF_H

#define BUFLEN 65536

#include <assert.h>

typedef struct fb_t_ {
	char start[BUFLEN];
	char *cursor;
	char *end;
	int fd;
} fb_t;

void fb_init(fb_t *fb, int fd);

void fb_flush(fb_t *fb);

void fb_puts(fb_t *fb, const char *s);

void fb_putc(fb_t *fb, char c);

int fb_printf_hack(fb_t *fb, size_t maxlen, const char *fmt, ...);

void fb_u(fb_t *fb, uint32_t x, int pad, char padc);

int fb_fp(fb_t *f, long double y, int w, int p);

static inline void fb_write(fb_t *fb, const char *b, size_t len) {
	assert(len < BUFLEN);
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
