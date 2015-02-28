#define _DEFAULT_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "util.h"
#include "fbuf.h"
#include "time.h"
#include "conf.h"

#define second 1
#define minute (60 * second)
#define hour   (60 * minute)
#define day    (24 * hour)
#define week   (7  * day)
#define month  (30 * day)
#define year   (12 * month)

uint32_t current_time(void) {
	struct timespec t;
	if (clock_gettime(CLOCK_REALTIME, &t) == -1) {
		die_errno();
	}
	return ts_to_u32(t);
}

inline void reltime(fb_t *out, const uint32_t now, const uint32_t then) {
	const uint64_t diff = now - then;
	if (diff <= second) {
		fb_ws(out, cSecond);
		fb_ws(out, "  <s" cEnd);
	} else if (diff < minute) {
		fb_ws(out, cSecond);
		fb_u(out, diff, 3, ' ');
		fb_ws(out, "s" cEnd);
	} else if (diff < hour) {
		fb_ws(out, cMinute);
		fb_u(out, diff/minute, 3, ' ');
		fb_ws(out, "m" cEnd);
	} else if (diff < hour*36) {
		fb_ws(out, cHour);
		fb_u(out, diff/hour, 3, ' ');
		fb_ws(out, "h" cEnd);
	} else if (diff < month) {
		fb_ws(out, cDay);
		fb_u(out, diff/day, 3, ' ');
		fb_ws(out, "d" cEnd);
	} else if (diff < year) {
		fb_ws(out, cWeek);
		fb_u(out, diff/week, 3, ' ');
		fb_ws(out, "w" cEnd);
	} else {
		fb_ws(out, cYear);
		fb_u(out, diff/year, 3, ' ');
		fb_ws(out, "y" cEnd);
	}
}

/*
func reltimeNoColor(b writer, then int64) {
	const f = "%3d"
	diff := (now - then) / 1e9

	if (diff <= second) {
		b.Write(nSecond)
		b.Write([]byte("  <s"))

	} else if (diff < minute) {
		b.Write(nSecond)
		fmt.Fprintf(b, f, diff)
		b.WriteByte('s')

	} else if (diff < hour) {
		b.Write(nMinute)
		fmt.Fprintf(b, f, diff/minute)
		b.WriteByte('m')

	} else if (diff < hour*36) {
		b.Write(nHour)
		fmt.Fprintf(b, f, diff/hour)
		b.WriteByte('h')

	} else if (diff < month) {
		b.Write(nDay)
		fmt.Fprintf(b, f, diff/day)
		b.WriteByte('d')

	} else if (diff < year) {
		b.Write(nWeek)
		fmt.Fprintf(b, f, diff/week)
		b.WriteByte('w')

	//} else if (diff < Year) {
	//	b.Write(cMonth)
	//	fmt.Fprintf(b, f, diff/Month) +
	//	b.WriteByte('y')
	//	b.Write(cEnd)
	
	} else {
		b.Write(nYear)
		fmt.Fprintf(b, f, diff/year)
		b.WriteByte('y')
	}
}
*/
