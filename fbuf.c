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

void fb_init(fb_t *fb, int fd) {
	fb->fd = fd;
	fb->end = fb->start+BUFLEN-1;
	fb->cursor = fb->start;
}

void fb_flush(fb_t *fb) {
	ssize_t l = fb->cursor-fb->start;
	ssize_t c = 0;
	do {
		ssize_t w = write(fb->fd, fb->start+c, l-c);
		assert(w != -1);
		c += w;
	} while (l-c > 0);
	fb->cursor = fb->start;
}

void fb_puts(fb_t *fb, const char *b) {
	fb_write(fb, b, strlen(b));
}

void fb_putc(fb_t *fb, char c) {
	if (fb->end - fb->cursor <= 0) {
		fb_flush(fb);
	}
	*fb->cursor++ = c;
}

//
// Number formatting
//

// XXX: merge with fmt_u
void fb_u(fb_t *fb, uint32_t x, int pad, char padc) {
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

//
// XXX: simplify massively
//

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

#define ZERO_PAD   (1U<<('0'-' '))
#define LEFT_ADJ   (1U<<('-'-' '))

static void pad(fb_t *f, char c, int w, int l, int fl) {
	char pad[256];
	if (fl & (LEFT_ADJ | ZERO_PAD) || l >= w) return;
	l = w - l;
	memset(pad, c, l>sizeof pad ? sizeof pad : l);
	for (; l >= sizeof pad; l -= sizeof pad) {
		fb_write(f, pad, sizeof pad);
	}
	fb_write(f, pad, l);
}

char *fmt_u(uintmax_t x, char *s);
inline char *fmt_u(uintmax_t x, char *s) {
	for (; x != 0; x/=10) {
		*--s = '0' + x%10;
	}
	return s;
}

/* Do not override this check. The floating point printing code below
 * depends on the float.h constants being right. If they are wrong, it
 * may overflow the stack. */
#if LDBL_MANT_DIG == 53
typedef char compiler_defines_long_double_incorrectly[9-(int)sizeof(long double)];
#endif

int fb_fp(fb_t *f, long double y, int w, int p) {
	uint32_t big[(LDBL_MANT_DIG+28)/29 + 1	    // mantissa expansion
		+ (LDBL_MAX_EXP+LDBL_MANT_DIG+28+8)/9]; // exponent expansion
	uint32_t *a, *d, *r, *z;
	int e2=0, e, i, j, l;
	char buf[9+LDBL_MANT_DIG/4];
	const char *prefix="-0X+0X 0X-0x+0x 0x";
	int pl;

	pl=1;
	if (signbit(y)) {
		y=-y;
	} else prefix++, pl=0;

	if (!isfinite(y)) {
		char *s = "inf";
		if (y!=y) s="nan";
		pad(f, ' ', w, 3+pl, 0);
		fb_write(f, prefix, pl);
		fb_write(f, s, 3);
		pad(f, ' ', w, 3+pl, LEFT_ADJ);
		return MAX(w, 3+pl);
	}

	y = frexpl(y, &e2) * 2;
	if (y) e2--;

	if (p<0) p=6;

	if (y) y *= 0x1p28, e2-=28;

	if (e2<0) a=r=z=big;
	else a=r=z=big+sizeof(big)/sizeof(*big) - LDBL_MANT_DIG - 1;

	do {
		*z = (uint32_t)y;
		y = 1000000000*(y-*z++);
	} while (y);

	while (e2>0) {
		uint32_t carry=0;
		int sh=MIN(29,e2);
		for (d=z-1; d>=a; d--) {
			uint64_t x = ((uint64_t)*d<<sh)+carry;
			*d = x % 1000000000;
			carry = x / 1000000000;
		}
		if (carry) *--a = carry;
		while (z>a && !z[-1]) z--;
		e2-=sh;
	}
	while (e2<0) {
		uint32_t carry=0;
		int sh=MIN(9,-e2), need=1+(p+LDBL_MANT_DIG/3+8)/9;
		for (d=a; d<z; d++) {
			uint32_t rm = *d & ((1<<sh)-1);
			*d = (*d>>sh) + carry;
			carry = (1000000000>>sh) * rm;
		}
		if (!*a) a++;
		if (carry) *z++ = carry;
		/* Avoid (slow!) computation past requested precision */
		if (z-r > need) z = r+need;
		e2+=sh;
	}

	if (a<z) for (i=10, e=9*(r-a); *a>=i; i*=10, e++);
	else e=0;

	/* Perform rounding: j is precision after the radix (possibly neg) */
	j = p;
	if (j < 9*(z-r-1)) {
		uint32_t x;
		/* We avoid C's broken division of negative numbers */
		d = r + 1 + ((j+9*LDBL_MAX_EXP)/9 - LDBL_MAX_EXP);
		j += 9*LDBL_MAX_EXP;
		j %= 9;
		for (i=10, j++; j<9; i*=10, j++);
		x = *d % i;
		/* Are there any significant digits past j? */
		if (x || d+1!=z) {
			long double round = 2/LDBL_EPSILON;
			long double small;
			if (*d/i & 1) round += 2;
			if (x<i/2) small=0x0.8p0;
			else if (x==i/2 && d+1==z) small=0x1.0p0;
			else small=0x1.8p0;
			if (pl && *prefix=='-') round*=-1, small*=-1;
			*d -= x;
			/* Decide whether to round by probing round+small */
			if (round+small != round) {
				*d = *d + i;
				while (*d > 999999999) {
					*d--=0;
					if (d<a) *--a=0;
					(*d)++;
				}
				for (i=10, e=9*(r-a); *a>=i; i*=10, e++);
			}
		}
		if (z>d+1) z=d+1;
	}
	for (; z>a && !z[-1]; z--);

	l = 1 + p + p;
	if (e>0) l+=e;

	pad(f, ' ', w, pl+l, 0);
	fb_write(f, prefix, pl);
	pad(f, '0', w, pl+l, ZERO_PAD);

	if (a>r) a=r;
	for (d=a; d<=r; d++) {
		char *s = fmt_u(*d, buf+9);
		if (d!=a) while (s>buf) *--s='0';
		else if (s==buf+9) *--s='0';
		fb_write(f, s, buf+9-s);
	}
	if (p) fb_putc(f, '.');
	for (; d<z && p>0; d++, p-=9) {
		char *s = fmt_u(*d, buf+9);
		while (s>buf) *--s='0';
		fb_write(f, s, MIN(9,p));
	}
	pad(f, '0', p+9, 9, 0);

	pad(f, ' ', w, pl+l, LEFT_ADJ);

	return MAX(w, pl+l);
}
