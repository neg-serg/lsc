#ifndef LSC_FBUF_H
#define LSC_FBUF_H

typedef struct fb_t_ {
	size_t cap;
	char *cursor;
	char *start;
	char *end;
	int fd;
} fb_t;

fb_t fb_new(int fd);

void fb_free(fb_t *fb);

void fb_flush(fb_t *fb);

void fb_write(fb_t *fb, const char *b, size_t len);

void fb_puts(fb_t *fb, const char *s);

void fb_putc(fb_t *fb, char c);

int fb_printf_hack(fb_t *fb, size_t maxlen, const char *restrict fmt, ...);

void fb_u(fb_t *fb, uint32_t x, int pad, char padc);

#define fb_ws(fb, s) (fb_write((fb), (s), sizeof(s)-1))

#endif
