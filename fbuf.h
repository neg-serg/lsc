typedef struct fb {
	char *start;
	char *end;
	char *cursor;
	size_t len;
	int fd;
} fb;

void fb_init(fb *f, int fd, size_t buflen);
void fb_drop(fb *f);
void fb_write(fb *f, const char *b, size_t len);
void fb_write_buf(fb *f, buf b);
void fb_puts(fb *f, const char *b);
void fb_flush(fb *f);
void fb_u(fb *f, uint32_t x, int pad, char padc);

void fb_putc(fb *fb, char c);

#define fb_write_const(fb, s) fb_write((fb), (s), sizeof(s)-1)
