/* TODO
 * refactor width stuff
 * fix potential verrevcmp overflow
 * naming, code organization
 * redesign cli
 * config file
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <locale.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#include "config.h"

#define program_name "lsc"

#define log(fmt, ...) (assertx(fprintf(stderr, fmt "\n", __VA_ARGS__) >= 0))
#define warn(fmt, ...) (log("%s: " fmt, program_name, __VA_ARGS__))
#define die(fmt, ...) do { warn(fmt, __VA_ARGS__); exit(1); } while (0)
#define warn_errno(fmt, ...) warn(fmt ": %s", __VA_ARGS__, strerror(errno))
#define die_errno(fmt, ...) die(fmt ": %s", __VA_ARGS__, strerror(errno))

#define assertx(expr) (expr?(void)0:abort())

#define MAX(x, y) ((x)>(y)?(x):(y))
#define MIN(x, y) ((x)<(y)?(x):(y))

#define ls_isalpha(c) (((unsigned)(c)|32)-'a' < 26)
#define ls_isdigit(c) ((unsigned)(c)-'0' < 10)

static inline size_t size_mul(size_t a, size_t b) {
    if (b > 1 && SIZE_MAX / b < a) abort();
	return a * b;
}

static inline void *xmalloc(size_t nmemb, size_t size) {
	void *p = malloc(size_mul(nmemb, size));
	assertx(p);
	return p;
}

static inline void *xrealloc(void *p, size_t nmemb, size_t size) {
	p = realloc(p, size_mul(nmemb, size));
	assertx(p);
	return p;
}

enum sort_type { SORT_FVER, SORT_SIZE, SORT_TIME };
enum uinfo_type { UINFO_NEVER, UINFO_AUTO, UINFO_ALWAYS };
enum date_type { DATE_NONE, DATE_REL, DATE_ABS };
enum layout_type { LAYOUT_GRID_COLUMNS, LAYOUT_GRID_LINES, LAYOUT_1LINE };

static struct {
	bool all;
	bool dir;
	bool m_time;
	bool total;
	// sorting
	bool no_group_dir;
	bool reverse;
	enum sort_type sort;
	// data/formatting
	enum layout_type layout;
	bool follow_links;
	bool strmode;
	enum uinfo_type userinfo;
	enum date_type date;
	bool size;
	bool no_classify;
} options;

typedef struct {
	const char *name, *linkname;
	mode_t mode, linkmode;
	id_t uid, gid;
	time_t time;
	off_t size;
	uint16_t name_len, linkname_len;
	uint16_t uwidth, gwidth, nwidth;
	uint16_t name_suf;
	bool linkok;
} file_info;

static void fi_free(file_info *fi) {
	if (fi->name) free((void *)fi->name);
	if (fi->linkname) free((void *)fi->linkname);
}

static int order(char c) {
	if (ls_isalpha(c)) return c;
	if (ls_isdigit(c)) return 0;
	if (c == '~') return -1;
	return (int)c + 256;
}

static int verrevcmp(const char *a, const char *b, size_t al, size_t bl) {
	size_t ai = 0, bi = 0;
	while (ai < al || bi < bl) {
		int first_diff = 0;
		// XXX: heap overflow stuff with afl/asan
		while ((ai < al && !ls_isdigit(a[ai])) ||
		       (bi < bl && !ls_isdigit(b[bi]))) {
			int ac = (ai == al) ? 0 : order(a[ai]);
			int bc = (bi == bl) ? 0 : order(b[bi]);
			if (ac != bc) return ac - bc;
			ai++; bi++;
		}
		while (a[ai] == '0') ai++;
		while (b[bi] == '0') bi++;
		while (ls_isdigit(a[ai]) && ls_isdigit(b[bi])) {
			if (!first_diff) first_diff = a[ai] - b[bi];
			ai++; bi++;
		}
		if (ls_isdigit(a[ai])) return 1;
		if (ls_isdigit(b[bi])) return -1;
		if (first_diff) return first_diff;
	}
	return 0;
}

// read file extension
// ^\.?.*?(\.[A-Za-z~][A-Za-z0-9~])*$
size_t suf_index(const char *s, size_t len) {
	if (len != 0 && s[0] == '.') { s++; len--; }
	bool alpha = false;
	size_t match = 0;
	for (size_t j = 0; j < len; j++) {
		char c = s[len - j - 1];
		if (ls_isalpha(c) || c == '~')
			alpha = true;
		else if (alpha && c == '.')
			match = j + 1;
		else if (ls_isdigit(c))
			alpha = false;
		else
			break;
	}
	return len - match;
}


static int filevercmp(const char *a, size_t al, size_t ai,
	const char *b, size_t bl, size_t bi)
{
	if (!al || !bl) return !al - !bl;
	int s = strcmp(a, b);
	if (!s) return 0;
	if (a[0] == '.' && b[0] != '.') return -1;
	if (a[0] != '.' && b[0] == '.') return 1;
	if (a[0] == '.' && b[0] == '.') a++, al--, b++, bl--;
	if (ai == bi && !strncmp(a, b, ai)) {
		a += ai; ai = al - ai;
		b += bi; bi = bl - bi;
	}
	int r = verrevcmp(a, b, ai, bi);
	return r ? r : s;
}

#define fi_isdir(fi) (S_ISDIR((fi)->mode) || S_ISDIR((fi)->linkmode))

static inline int fi_cmp(const void *va, const void *vb) {
	file_info *a = (file_info *const)va;
	file_info *b = (file_info *const)vb;
	int rev = options.reverse ? -1 : 1;
	if (!options.no_group_dir)
		if (fi_isdir(a) != fi_isdir(b))
			return fi_isdir(a) ? -1 : 1;
	if (options.sort == SORT_SIZE) {
		off_t s = a->size - b->size;
		if (s) return rev * ((s > 0) - (s < 0));
	}
	if (options.sort == SORT_TIME) {
		time_t t = a->time - b->time;
		if (t) return rev * ((t > 0) - (t < 0));
	}
	return rev * filevercmp(a->name, a->name_len, a->name_suf,
		b->name, b->name_len, b->name_suf);
}

// file info vector
typedef struct {
	file_info *data;
	size_t cap, len;
	int nwidth, uwidth, gwidth;
	bool userinfo;
	id_t uid, gid;
} file_list;

static void fv_clear(file_list *v) {
	v->nwidth = v->uwidth = v->gwidth = 0;
	v->userinfo = options.userinfo == UINFO_ALWAYS;
	v->len = 0;
}

static void fv_init(file_list *v, size_t init) {
	v->data = xmalloc(init, sizeof(file_info));
	v->cap = init;
	fv_clear(v);
}

static file_info *fv_index(file_list *v, size_t i) { return &v->data[i]; }

static file_info *fv_stage(file_list *v) {
	if (v->len >= v->cap) {
		v->cap = size_mul(v->cap, 2);
		v->data = xrealloc(v->data, v->cap, sizeof(file_info));
	}
	return fv_index(v, v->len);
}

static void fv_commit(file_list *v) { v->len++; }

// read symlink target
static const char *ls_readlink(int dirfd, const char *name, size_t size) {
	char *buf = xmalloc(size + 1, 1); // allocate length + \0
	ssize_t n = readlinkat(dirfd, name, buf, size);
	if (n == -1) {
		free(buf);
		return 0;
	}
	assertx((size_t)n == size); // possible truncation
	buf[n] = '\0';
	return buf;
}

// populates file_info with file information
static int ls_stat(file_list *l, file_info *fi, int dirfd, char *name) {
	fi->name = name;
	fi->name_len = strlen(name);
	fi->name_suf = suf_index(name, fi->name_len);
	fi->linkname = 0;
	fi->linkok = true;
	struct stat st;
	if (fstatat(dirfd, name, &st, AT_SYMLINK_NOFOLLOW) == -1)
		return -1;
	fi->mode = st.st_mode;
	fi->time = options.m_time ? st.st_mtime : st.st_ctime;
	fi->size = st.st_size;
	fi->uid = st.st_uid;
	fi->gid = st.st_gid;
	if (options.userinfo == UINFO_AUTO)
		l->userinfo |= st.st_uid != l->uid || st.st_gid != l->gid;
	if (S_ISLNK(fi->mode)) {
		const char *ln = ls_readlink(dirfd, name, st.st_size);
		if (!ln) { fi->linkok = false; return 0; }
		fi->linkname = ln;
		fi->linkname_len = (size_t)st.st_size;
		if (fstatat(dirfd, name, &st, 0) == -1) {
			fi->linkok = false;
			return 0;
		}
		fi->linkmode = st.st_mode;
	}
	return 0;
}

// list directory
static int ls_readdir(file_list *v, const char *name) {
	DIR *dir = opendir(name);
	if (!dir) {
		warn_errno("cannot open directory '%s'", name);
		return -1;
	}
	int fd = dirfd(dir);
	if (fd == -1) {
		warn_errno("%s", name);
		return -1;
	}
	struct dirent *dent;
	int err = 0;
	while ((dent = readdir(dir))) {
		const char *p = dent->d_name;
		if (p[0] == '.' && !options.all) continue;
		if (p[0] == '.' && p[1] == '\0') continue;
		if (p[0] == '.' && p[1] == '.' && p[2] == '\0') continue;
		file_info *out = fv_stage(v);
		char *dup = strdup(p);
		if (ls_stat(v, out, fd, dup) == -1) {
			free(dup);
			err = -1;
			warn_errno("cannot access '%s/%s'", name, p);
			continue;
		}
		fv_commit(v);
	}
	if (closedir(dir) == -1)
		return -1;
	return err;
}

// list file/directory
static int ls(file_list *v, const char *name) {
	file_info *out = fv_stage(v); // new uninitialized file_info
	char *dup = strdup(name);
	if (ls_stat(v, out, AT_FDCWD, dup) == -1) {
		free(dup);
		warn_errno("cannot access '%s'", name);
		return -1;
	}
	if (!options.dir && fi_isdir(out)) {
		free(dup);
		return ls_readdir(v, name);
	}
	fv_commit(v);
	return 0;
}

enum ls_color_labels {
	L_LEFT, L_RIGHT, L_END, L_RESET, L_NORM, L_FILE, L_DIR, L_LINK, L_FIFO,
	L_SOCK, L_BLK, L_CHR, L_MISSING, L_ORPHAN, L_EXEC, L_DOOR, L_SETUID,
	L_SETGID, L_STICKY, L_OW, L_STICKYOW, L_CAP, L_MULTIHARDLINK, L_CLR_TO_EOL,
	L_LENGTH,
};

struct lsc_pair {
	const char *ext;
	const char *color;
};

struct {
	char *labels[L_LENGTH];
	struct lsc_pair *map;
	size_t exts;
} ls_colors;

static const char *const lsc_labels[] = {
	"lc", "rc", "ec", "rs", "no", "fi", "di", "ln",
	"pi", "so", "bd", "cd", "mi", "or", "ex", "do",
	"su", "sg", "st", "ow", "tw", "ca", "mh", "cl", NULL,
};

static int lsc_cmp(const void *va, const void *vb) {
	struct lsc_pair *a = (struct lsc_pair *const)va;
	struct lsc_pair *b = (struct lsc_pair *const)vb;
	return strcmp(a->ext, b->ext);
}

static const char *lsc_lookup(const char *ext) {
	struct lsc_pair k = { .ext = ext, .color = NULL }, *res;
	res = bsearch(&k, ls_colors.map, ls_colors.exts, sizeof(k), lsc_cmp);
	return res ? res->color : NULL;
}

static void lsc_parse(char *lsc_env) {
	size_t exts = 0, exti = 0;
	size_t len = strlen(lsc_env);
	for (size_t i = 0; i < len; i++) if (lsc_env[i] == '*') exts++;
	ls_colors.map = xmalloc(exts, sizeof(*ls_colors.map));
	bool eq = false;
	size_t kbegin = 0, kend = 0;
	for (size_t i = 0; i < len; i++) {
		char c = lsc_env[i];
		if (c == '=') { kend = i; eq = true; continue; }
		if (!eq || c != ':') continue;
		lsc_env[kend] = lsc_env[i] = '\0';
		char *k = lsc_env + kbegin;
		char *v = lsc_env + kend + 1;
		if (*k == '*')
			ls_colors.map[exti++] = (struct lsc_pair) { k + 1, v };
		else if (kend - kbegin == 2)
			for (size_t i = 0; i < L_LENGTH; i++)
				if (k[0] == lsc_labels[i][0] && k[1] == lsc_labels[i][1]) {
					ls_colors.labels[i] = v;
					break;
				}
		kbegin = i + 1;
		i += 2;
		eq = false;
	}
	ls_colors.exts = exti;
	qsort(ls_colors.map, ls_colors.exts, sizeof(*ls_colors.map), lsc_cmp);
}

struct idcache { struct idcache *next; id_t id; char name[]; };

static const char *id_put(struct idcache **cache, id_t id, const char *name) {
	struct idcache *p = xmalloc(sizeof(*p) + strlen(name) + 1, 1);
	strcpy(p->name, name);
	p->id = id;
	p->next = *cache, *cache = p;
	return p->name[0] ? p->name : 0;
}

static struct idcache *ucache;

static const char *getuser(uid_t id) {
	for (struct idcache *p = ucache; p; p = p->next)
		if (p->id == id) return p->name[0] ? p->name : 0;
	struct passwd *e = getpwuid(id);
	return id_put(&ucache, id, e ? e->pw_name : "");
}

static struct idcache *gcache;

static const char *getgroup(gid_t id) {
	for (struct idcache *p = gcache; p; p = p->next)
		if (p->id == id) return p->name[0] ? p->name : 0;
	struct group *e = getgrgid(id);
	return id_put(&ucache, id, e ? e->gr_name : "");
}

static void fmt_strmode(FILE *out, const mode_t mode) {
	switch (mode&S_IFMT) {
	case S_IFREG:  fputs(C_FILE,    out); break;
	case S_IFDIR:  fputs(C_DIR,     out); break;
	case S_IFCHR:  fputs(C_CHAR,    out); break;
	case S_IFBLK:  fputs(C_BLOCK,   out); break;
	case S_IFIFO:  fputs(C_FIFO,    out); break;
	case S_IFLNK:  fputs(C_LINK,    out); break;
	case S_IFSOCK: fputs(C_SOCK,    out); break;
	default:       fputs(C_UNKNOWN, out); break;
	}
	fputs(mode&S_IRUSR ? C_READ : C_NONE, out);
	fputs(mode&S_IWUSR ? C_WRITE : C_NONE, out);
	fputs(mode&S_ISUID ? mode&S_IXUSR ? C_UID_EXEC : C_UID
	                   : mode&S_IXUSR ? C_EXEC : C_NONE, out);
	fputs(mode&S_IRGRP ? C_READ : C_NONE, out);
	fputs(mode&S_IWGRP ? C_WRITE : C_NONE, out);
	fputs(mode&S_ISGID ? mode&S_IXGRP ? C_UID_EXEC : C_UID
	                   : mode&S_IXGRP ? C_EXEC : C_NONE, out);
	fputs(mode&S_IROTH ? C_READ : C_NONE, out);
	fputs(mode&S_IWOTH ? C_WRITE : C_NONE, out);
	fputs(mode&S_ISVTX ? mode&S_IXOTH ? C_STICKY : C_STICKY_O
	                   : mode&S_IXOTH ? C_EXEC : C_NONE, out);
	putc(' ', out);
}

#define SECOND 1
#define MINUTE (60 * SECOND)
#define HOUR   (60 * MINUTE)
#define DAY    (24 * HOUR)
#define WEEK   (7  * DAY)
#define MONTH  (30 * DAY)
#define YEAR   (12 * MONTH)

time_t now;

static void get_current_time(void) {
	struct timespec t;
	if (clock_gettime(CLOCK_REALTIME, &t) == -1)
		die_errno("%s", "current_time");
	now = t.tv_sec;
}

static void fmt_abstime(FILE *out, const time_t then) {
	time_t diff = now - then;
	char buf[20];
	struct tm tm;
	localtime_r(&then, &tm);
	char *fmt = diff < MONTH * 6 ? "%e %b %H:%M" : "%e %b  %Y";
	strftime(buf, sizeof(buf), fmt, &tm);
	fputs(C_DAY, out);
	fputs(buf, out);
	putc(' ', out);
}

static void fmt3(char b[static 3], uint16_t x) {
	if (x/100) b[0] = '0' + x/100;
	if (x/100||x/10%10) b[1] = '0' + x/10%10;
	b[2] = '0' + x%10;
}

static void fmt_reltime(FILE *out, const time_t then) {
	time_t diff = now - then;
	if (diff < 0) {
		fputs(C_SECOND " 0s " C_END, out);
		return;
	}
	if (diff <= SECOND) {
		fputs(C_SECOND "<1s " C_END, out);
		return;
	}
	char b[4] = "  0s";
	if (diff < MINUTE) {
		fputs(C_SECOND, out);
	} else if (diff < HOUR) {
		fputs(C_MINUTE, out);
		diff /= MINUTE;
		b[3] = 'm';
	} else if (diff < HOUR*36) {
		fputs(C_HOUR, out);
		diff /= HOUR;
		b[3] = 'h';
	} else if (diff < MONTH) {
		fputs(C_DAY, out);
		diff /= DAY;
		b[3] = 'd';
	} else if (diff < YEAR) {
		fputs(C_WEEK, out);
		diff /= WEEK;
		b[3] = 'w';
	} else {
		fputs(C_YEAR, out);
		diff /= YEAR;
		b[3] = 'y';
	}
	fmt3(b, diff);
	fwrite(b+1, 1, 3, out);
	putc(' ', out);
}

static const char *const C_SIZES[7] = { "B", "K", "M", "G", "T", "P", "E" };

static off_t divide(off_t x, off_t d) { return (x+(d-1)/2)/d; }

static void fmt_size(FILE *out, off_t sz) {
	fputs(C_SIZE, out);
	int m = 0;
	off_t div = 1, u = sz;
	while (u > 999) {
		div *= 1024;
		u = divide(u, 1024);
		m++;
	}
	off_t v = divide(sz*10, div);
	char b[3] = "  0";
	if (v/10 >= 10 || m == 0) {
		fmt3(b, u);
	} else if (sz != 0) {
		b[0] = '0' + v/10;
		b[1] = '.';
		b[2] = '0' + v%10;
	}
	fwrite(b, 1, 3, out);
	fputs(C_SIZES[m], out);
	putc(' ', out);
}

static int color_type(mode_t mode) {
	#define S_IXUGO (S_IXUSR|S_IXGRP|S_IXOTH)
	switch (mode&S_IFMT) {
	case S_IFREG:
		if (mode&S_ISUID) return L_SETUID;
		if (mode&S_ISGID) return L_SETGID;
		if (mode&S_IXUGO) return L_EXEC;
		return L_FILE;
	case S_IFDIR:
		if (mode&S_ISVTX && mode&S_IWOTH) return L_STICKYOW;
		if (mode&S_IWOTH) return L_OW;
		if (mode&S_ISVTX) return L_STICKY;
		return L_DIR;
	case S_IFLNK: return L_LINK;
	case S_IFIFO: return L_FIFO;
	case S_IFSOCK: return L_SOCK;
	case S_IFCHR: return L_CHR;
	case S_IFBLK: return L_BLK;
	default: return L_ORPHAN;
	}
}

static const char *suf_color(const char *name, size_t len) {
	while (len--)
		if (name[len] == '.')
			return lsc_lookup(name + len);
	return 0;
}

static const char *file_color(const char *name, size_t len, int t) {
	if (t == L_FILE || t == L_LINK) {
		const char *c = suf_color(name, len);
		if (c) return c;
	}
	return ls_colors.labels[t];
}

static int strwidth(const char *s) {
	mbstate_t st = {0};
	wchar_t wc;
	int len = strlen(s), w = 0, i = 0;
	while (i < len) {
		int c = (unsigned char)s[i];
		if (c <= 0x7f) { // ascii fast path
			if (0x7f > c && c > 0x1f) { w++, i++; }
			continue;
		}
		int r = mbrtowc(&wc, s+i, len-i, &st);
		if (r < 0) break;
		w += wcwidth(wc);
		i += r;
	}
	return w;
}

static int fmt_name_width(const file_info *fi) {
	int w = strwidth(fi->name);
	if (options.follow_links && fi->linkname) {
		w += 1 + strlen(C_SYM_DELIM) + strwidth(fi->linkname);
	}
	if (!options.no_classify) {
		mode_t m = fi->linkname && options.follow_links ? fi->linkmode : fi->mode;
		w += (S_ISREG(m) && m&S_IXUGO) || S_ISDIR(m) || S_ISLNK(m) ||
			S_ISFIFO(m) || S_ISSOCK(m);
	}
	return w;
}

static void fmt_name(FILE *out, const file_info *fi) {
	int t;
	const char *c;
	if (fi->linkname && options.follow_links) {
		t = fi->linkok ? color_type(fi->linkmode) : L_ORPHAN;
		c = file_color(fi->linkname, fi->linkname_len, t);
	} else {
		t = color_type(fi->mode);
		c = file_color(fi->name, fi->name_len, t);
	}
	fputs(C_ESC, out);
	fputs(c ? c : "0", out);
	fputs("m", out);
	fwrite(fi->name, 1, fi->name_len, out);
	if (c) fputs(C_END, out);
	if (options.follow_links && fi->linkname) {
		fputs(" " C_SYM_DELIM_COLOR C_SYM_DELIM C_ESC, out);
		fputs(c ? c : "0", out);
		fputs("m", out);
		fwrite(fi->linkname, 1, fi->linkname_len, out);
		if (c) fputs(C_END, out);
	}
	if (!options.no_classify) {
		mode_t m = fi->linkname && options.follow_links ? fi->linkmode : fi->mode;
		if (S_ISREG(m) && m&S_IXUGO) fputs(CL_EXEC, out);
		else if S_ISDIR(m) fputs(CL_DIR, out);
		else if S_ISLNK(m) fputs(CL_LINK, out);
		else if S_ISFIFO(m) fputs(CL_FIFO, out);
		else if S_ISSOCK(m) fputs(CL_SOCK, out);
	}
}

static void fmt_usergroup(FILE *out, id_t id, const char *n, int w, int mw) {
	if (n) fputs(n, out);
	else fprintf(out, "%d", id);
	for (int n = mw - w + 1; n--;)
		putc(' ', out);
}

static void fmt_userinfo(FILE *out, file_list *l, file_info *fi) {
	fputs(C_USERINFO, out);
	fmt_usergroup(out, fi->uid, getuser(fi->uid), fi->uwidth, l->uwidth);
	fmt_usergroup(out, fi->gid, getgroup(fi->gid), fi->gwidth, l->gwidth);
}

static int fmt_file_width(file_list *l, file_info *fi) {
	int w = 0;
	if (options.strmode)
		w += 10 + 1;
	if (l->userinfo)
		w += fi->uwidth + 1 + fi->gwidth + 1;
	if (options.date == DATE_ABS)
		w += 12 + 1;
	if (options.date == DATE_REL)
		w += 3 + 1;
	if (options.size)
		w += 4 + 1;
	return w + fi->nwidth;
}

static void fmt_file(FILE *out, file_list *l, file_info *fi) {
	if (options.strmode)
		fmt_strmode(out, fi->mode);
	if (l->userinfo)
		fmt_userinfo(out, l, fi);
	if (options.date == DATE_ABS)
		fmt_abstime(out, fi->time);
	if (options.date == DATE_REL)
		fmt_reltime(out, fi->time);
	if (options.size)
		fmt_size(out, fi->size);
	fmt_name(out, fi);
}

struct grid { int *columns, x, y; };

bool grid_layout(struct grid *g, int direction, int padding, int term_width,
	int max_width, int *widths, int widths_len)
{
	g->columns = 0;
	int *cols = 0;
	// iterate through numbers of rows, starting at upper bound
	int n = (term_width - max_width) / (padding + max_width) + 1;
	int upper_bound = (widths_len + n - 1) / n + 1;
	for (int r = upper_bound; r >= 1; r--) {
		// calculate number of columns for rows
		int c = (widths_len + r - 1) / r;
		// skip uninteresting rows
		r = ((widths_len + c - 1) / c);
		// total padding between columns
		int total_separator_width = (c - 1) * padding;
		// find maximum width in each column
		cols = xrealloc(cols, c, sizeof(*cols));
		memset(cols, 0, c * sizeof(*cols));
		for (int i = 0; i < widths_len; i++) {
			int ci = direction ? i % c : i / r;
			cols[ci] = MAX(cols[ci], widths[i]);
		}
		// calculate total width of columns
		int total = 0;
		for (int i = 0; i < c; i++) total += cols[i];
		// check if columns fit
		if (total > term_width - total_separator_width)
			break;
		// store last layout that fits
		int *tmp = g->columns;
		g->columns = cols, cols = tmp;
		g->x = c;
		g->y = r;
	}
	if (cols) free(cols);
	return !!g->columns;
}

static void fmt_file_list(FILE *out, file_list *v) {
	if (v->userinfo)
		for (size_t i = 0; i < v->len; i++) {
			file_info *fi = fv_index(v, i);
			const char *u = getuser(fi->uid);
			const char *g = getgroup(fi->gid);
			fi->uwidth = u ? strwidth(u) : snprintf(0, 0, "%d", fi->uid);
			fi->gwidth = g ? strwidth(g) : snprintf(0, 0, "%d", fi->gid);
			v->uwidth = MAX(fi->uwidth, v->uwidth);
			v->gwidth = MAX(fi->gwidth, v->gwidth);
		}
	if (options.layout == LAYOUT_1LINE)
		goto oneline;
	int *widths = xmalloc(v->len, sizeof(int)), max_width = 0;
	for (size_t i = 0; i < v->len; i++) {
		file_info *fi = fv_index(v, i);
		fi->nwidth = fmt_name_width(fi);
		widths[i] = fmt_file_width(v, fi);
		max_width = MAX(max_width, widths[i]);
	}
	struct winsize w;
	int term_width = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 ? 80 : w.ws_col;
	if (term_width < max_width) {
		free(widths);
		goto oneline;
	}
	int direction = options.layout == LAYOUT_GRID_LINES, padding = 2;
	struct grid g = {0};
	bool grid = grid_layout(&g, direction, padding, term_width,
		max_width, widths, v->len);
	if (!grid) goto oneline;
	for (int y = 0; y < g.y; y++) {
		for (int x = 0; x < g.x; x++) {
			int i = direction ? y * g.x + x : g.y * x + y;
			if (i >= (int)v->len) continue;
			file_info *fi = fv_index(v, i);
			fmt_file(out, v, fi);
			int p = g.columns[x] - widths[i] + padding;
			while (p--) putc(' ', out);
			fi_free(fi);
		}
		putc('\n', out);
	}
	free(g.columns);
	free(widths);
	return;
oneline:
	for (size_t i = 0; i < v->len; i++) {
		file_info *fi = fv_index(v, i);
		fmt_file(out, v, fi);
		fi_free(fi);
		putc('\n', out);
	}
}

void usage(void) {
	log("Usage: %s [option ...] [file ...]"
		"\nTODO"
		/* "\n  -a  show all files" */
		/* "\n  -c  use ctime instead of mtime" */
		/* "\n  -G  group directories first" */
		/* "\n  -r  reverse sort" */
		/* "\n  -S  sort by file size" */
		/* "\n  -t  sort by mtime/ctime" */
		/* "\n  -1  list one file per line" */
		/* "\n  -g  show output in grid, by columns (default)" */
		/* "\n  -x  show output in grid, by lines" */
		/* "\n  -m  print file modes" */
		/* "\n  -u  print user and group info (automatic)" */
		/* "\n  -U  print user and group info (always)" */
		/* "\n  -d  print relative modification time" */
		/* "\n  -D  print absolute modification time" */
		/* "\n  -z  print file size" */
		/* "\n  -y  print symlink target" */
		/* "\n  -F  print type indicator" */
		/* "\n  -?  show this help" */
		, program_name);
}

int main(int argc, char **argv) {
	setlocale(LC_ALL, "");
	int c;
	while ((c = getopt(argc, argv, "aIMGrst1gxmdDuUzFyl?")) != -1)
		switch (c) {
		case 'a': options.all = true; break;
		case 'I': options.dir = true; break;
		case 'M': options.m_time = true; break;
		case 'G': options.no_group_dir = true; break;
		case 's': options.sort = SORT_SIZE; break;
		case 't': options.sort = SORT_TIME; break;
		case 'r': options.reverse = true; break;
		case '1': options.layout = LAYOUT_1LINE; break;
		case 'g': options.layout = LAYOUT_GRID_COLUMNS; break;
		case 'x': options.layout = LAYOUT_GRID_LINES; break;
		case 'm': options.strmode = true; break;
		case 'd': options.date = DATE_REL; break;
		case 'D': options.date = DATE_ABS; break;
		case 'u': options.userinfo = UINFO_AUTO; break;
		case 'U': options.userinfo = UINFO_ALWAYS; break;
		case 'z': options.size = true; break;
		case 'F': options.no_classify = true; break;
		case 'y': options.follow_links = true; break;
		case 'l':
			options.layout = LAYOUT_1LINE;
			options.date = DATE_REL;
			options.strmode = true;
			options.userinfo = UINFO_AUTO;
			options.follow_links = true;
			options.size = true;
			break;
		case '?': usage(); exit(EXIT_SUCCESS); break;
		default: exit(EXIT_FAILURE); break;
		}
	lsc_parse(getenv("LS_COLORS"));
	get_current_time();
	file_list v = {0};
	fv_init(&v, 64);
	v.uid = getuid();
	v.gid = getgid();
	if (optind >= argc) argv[--optind] = ".";
	int err = 0, arg_num = argc - optind;
	for (int i = 0; i < arg_num; i++) {
		char *path = argv[optind + i];
		err |= ls(&v, path) == -1;
		qsort(v.data, v.len, sizeof(*v.data), fi_cmp);
		if (arg_num > 1) {
			if (i) putchar('\n');
			printf("%s:\n", path);
		}
		fmt_file_list(stdout, &v);
		fv_clear(&v);
	};
	return err;
}
