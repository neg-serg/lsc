#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <search.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#define program_name "lsc"

#include "ls_colors.h"
#include "filevercmp.h"
#include "id.h"
#include "util.h"
#include "vec.h"
#include "config.h"

enum sort_type { SORT_FVER, SORT_SIZE, SORT_TIME };
enum uinfo_type { UINFO_NEVER, UINFO_AUTO, UINFO_ALWAYS };

static struct {
	bool all;
	// sorting
	bool group_dir;
	bool reverse;
	enum sort_type sort;
	// data/formatting
	bool classify;
	bool ctime;
	bool longdate;
	enum uinfo_type userinfo;
} options;

static uid_t myuid;
static gid_t mygid;

static bool uinfo_auto;

static int uwidth;
static int gwidth;

struct ls_colors ls_colors;

// big :(
typedef struct file_info {
	const char *name;
	const char *linkname;
	size_t name_len;
	size_t linkname_len;
	uid_t uid;
	gid_t gid;
	time_t time;
	off_t size;
	mode_t mode;
	mode_t linkmode;
	int name_suf;
	bool linkok;
} file_info;

static void fi_free(file_info *fi) {
	free((void *)fi->name);
	free((void *)fi->linkname);
}

static int fi_cmp(const void *va, const void *vb) {
	file_info *a = (file_info *const)va;
	file_info *b = (file_info *const)vb;
	int rev = options.reverse ? -1 : 1;
	if (options.group_dir) {
		if (S_ISDIR(a->linkmode) != S_ISDIR(b->linkmode))
			return S_ISDIR(a->linkmode) ? -1 : 1;
		if (S_ISDIR(a->mode) != S_ISDIR(b->mode))
			return S_ISDIR(a->mode) ? -1 : 1;
	}
	if (options.sort == SORT_SIZE) {
		ssize_t s = a->size - b->size;
		if (s) return rev * ((s > 0) - (s < 0));
	}
	if (options.sort == SORT_TIME) {
		time_t t = a->time - b->time;
		if (t) return rev * ((t > 0) - (t < 0));
	}
	return rev * filevercmp(a->name, b->name,
		a->name_len, b->name_len, a->name_suf, b->name_suf);
}

// file info vector
VEC(fv, file_vec, file_info)

// read symlink target
static ssize_t ls_readlink(int dirfd, const char *name, size_t size, const char **linkname) {
	char *buf = xmalloc(size + 1); // allocate length + \0
	ssize_t n = readlinkat(dirfd, name, buf, size);
	if (n == -1) {
		free(buf);
		return -1;
	}
	assertx((size_t)n <= size); // possible truncation
	buf[n] = '\0';
	*linkname = (const char *)buf;
	return n;
}

// populates file_info with file information
static int ls_stat(int dirfd, const char *name, file_info *out) {
	struct stat st;
	if (fstatat(dirfd, name, &st, AT_SYMLINK_NOFOLLOW) == -1)
		return -1;
	out->name = name;
	out->name_len = strlen(name);
	out->name_suf = suf_index(name, out->name_len);
	out->mode = st.st_mode;
	out->time = options.ctime ? st.st_ctime : st.st_mtime;
	out->size = st.st_size;
	out->uid = st.st_uid;
	out->gid = st.st_gid;
	if (options.userinfo == UINFO_AUTO)
		uinfo_auto |= st.st_uid != myuid || st.st_gid != mygid;
	out->linkname = NULL;
	out->linkname_len = 0;
	out->linkmode = 0;
	out->linkok = true;
	if (S_ISLNK(out->mode)) {
		ssize_t ln = ls_readlink(dirfd, name, st.st_size, &out->linkname);
		if (ln == -1) { out->linkok = false; return 0; }
		out->linkname_len = (size_t)ln;
		if (fstatat(dirfd, name, &st, 0) == -1) {
			out->linkok = false;
			return 0;
		}
		out->linkmode = st.st_mode;
	}
	return 0;
}

// list directory
static int ls_readdir(file_vec *v, const char *name) {
	DIR *dir = opendir(name);
	if (dir == NULL) {
		warn_errno("cannot open directory %s", name);
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
		struct file_info *out = fv_stage(v);
		char *dup = strdup(p);
		if (ls_stat(fd, dup, out) == -1) {
			free(dup);
			err = -1;
			warn_errno("cannot access %s/%s", name, p);
			continue;
		}
		fv_commit(v);
	}
	if (closedir(dir) == -1)
		return -1;
	return err;
}

// list file/directory
static int ls(file_vec *v, const char *name) {
	file_info *fi = fv_stage(v); // new uninitialized file_info
	char *s = strdup(name); // strdup to make fi_free work
	if (ls_stat(AT_FDCWD, s, fi) == -1) {
		free(s);
		warn_errno("cannot access %s", name);
		return -1;
	}
	if (S_ISDIR(fi->mode)) {
		free(s);
		return ls_readdir(v, name);
	}
	fv_commit(v);
	return 0;
}

//
// Formatting
//

static void fmt_strmode(FILE *out, const mode_t mode) {
	switch (mode&S_IFMT) {
	case S_IFREG:  fputs(C_FT_NONE, out); break;
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
}

#define SECOND 1
#define MINUTE (60 * SECOND)
#define HOUR   (60 * MINUTE)
#define DAY    (24 * HOUR)
#define WEEK   (7  * DAY)
#define MONTH  (30 * DAY)
#define YEAR   (12 * MONTH)

static time_t current_time(void) {
	struct timespec t;
	if (clock_gettime(CLOCK_REALTIME, &t) == -1)
		die_errno("%s", "current_time");
	return t.tv_sec;
}

static void fmt_longtime(FILE *out, const time_t now, const time_t then) {
	time_t diff = now - then;
	char buf[20];
	struct tm tm;
	localtime_r(&then, &tm);
	strftime(buf, sizeof(buf), 
		diff < MONTH * 6 ? "%e %b %H:%M" : "%e %b  %Y", &tm);
	putc(' ', out);
	fputs(C_DAY, out);
	fputs(buf, out);
}

static void fmt3(char b[3], uint16_t x) {
	if (x/100) b[0] = '0' + x/100;
	if (x/100||x/10%10) b[1] = '0' + x/10%10;
	b[2] = '0' + x%10;
}

static void fmt_reltime(FILE *out, const time_t now, const time_t then) {
	time_t diff = now - then;
	if (diff < 0) {
		fputs(C_SECOND "  0s" C_END, out);
		return;
	}
	if (diff <= SECOND) {
		fputs(C_SECOND " <1s" C_END, out);
		return;
	}
	char b[4] = { ' ', ' ', '0', 's' };
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
	fwrite(b, 1, 4, out);
	fputs(C_END, out);
}

static off_t divide(off_t x, off_t d) { return (x+((d-1)/2))/d; }

static void fmt_size(FILE *out, off_t sz) {
	fputs(C_SIZE, out);
	unsigned m = 0;
	off_t div = 1;
	off_t u = sz;
	while (u > 999) {
		div *= 1024;
		u = divide(u, 1024);
		m++;
	}
	off_t v = divide(sz*10, div);
	char b[3] = {' ', ' ', '0'};
	if (v/10 >= 10 || m == 0) {
		fmt3(b, u);
	} else if (sz != 0) {
		b[0] = '0' + v/10;
		b[1] = '.';
		b[2] ='0' + v%10;
	}
	fwrite(b, 1, 3, out);
	fputs(C_SIZES[m], out);
}

static enum labels color_type(mode_t mode) {
	#define S_IXUGO (S_IXUSR|S_IXGRP|S_IXOTH)
	switch (mode&S_IFMT) {
	case S_IFREG:
		if ((mode&S_ISUID) != 0)
			return L_SETUID;
		if ((mode&S_ISGID) != 0)
			return L_SETGID;
		if ((mode&S_IXUGO) != 0)
			return L_EXEC;
		return L_FILE;
	case S_IFDIR:
		if ((mode&S_ISVTX) != 0 && (mode&S_IWOTH) != 0)
			return L_STICKYOW;
		if ((mode&S_IWOTH) != 0)
			return L_OW;
		if ((mode&S_ISVTX) != 0)
			return L_STICKY;
		return L_DIR;
	case S_IFLNK:  return L_LINK;
	case S_IFIFO:  return L_FIFO;
	case S_IFSOCK: return L_SOCK;
	case S_IFCHR:  return L_CHR;
	case S_IFBLK:  return L_BLK;
	// anything else is classified as orphan
	default: return L_ORPHAN;
	}
}

static const char *immediate_files[] = {
	"Makefile", "Cargo.toml", "SConstruct", "CMakeLists.txt",
	"build.gradle", "Rakefile", "Gruntfile.js",
	"Gruntfile.coffee", "BUILD", "BUILD.bazel", "WORKSPACE", "build.xml",
	NULL,
};

static bool is_readme(const char *s) {
	for (char *c = "readme"; *s && *c; s++, c++)
		if (tolower(*s) != *c) return false;
	return true;
}

static bool is_immediate(const char *name) {
	for (const char **p = immediate_files; *p; p++)
		if (strcasecmp(name, *p) == 0) return true;
	return is_readme(name);
}

static const char *suf_color(const char *name, size_t len) {
	char *n = memrchr(name, '.', len);
	if (n == NULL) return NULL;
	return lsc_lookup(&ls_colors, n);
}

static const char *file_color(const char *name, size_t len, enum labels t) {
	if (t == L_FILE || t == L_LINK) {
		if (is_immediate(name))
			return ls_colors.labels[L_DOOR];
		const char *c = suf_color(name, len);
		if (c != NULL) return c;
	}
	return ls_colors.labels[t];
}

static void fmt_name(FILE *out, const struct file_info *f) {
	enum labels t;
	const char *c;
	if (f->linkname) {
		if (f->linkok) t = color_type(f->linkmode);
		else t = L_ORPHAN;
		c = file_color(f->linkname, f->linkname_len, t);
	} else {
		t = color_type(f->mode);
		c = file_color(f->name, f->name_len, t);
	}
	fputs(C_ESC, out);
	fputs(c, out);
	fputs("m", out);
	fwrite(f->name, 1, f->name_len, out);
	fputs(C_END, out);
	if (f->linkname) {
		fputs(C_SYM_DELIM C_ESC, out);
		fputs(c, out);
		fputs("m", out);
		fwrite(f->linkname, 1, f->linkname_len, out);
		fputs(C_END, out);
	}
	if (options.classify) {
		switch (t) {
		case L_DIR: case L_STICKYOW: case L_OW: case L_STICKY:
			fputs(CL_DIR, out); break;
		case L_EXEC: fputs(CL_EXEC, out); break;
		case L_FIFO: fputs(CL_FIFO, out); break;
		case L_SOCK: fputs(CL_SOCK, out); break;
		default: ;
		}
	}
}

static void fmt_user(FILE *out, uid_t uid, gid_t gid) {
	bool uinfo = (options.userinfo == UINFO_AUTO && uinfo_auto) ||
		options.userinfo == UINFO_ALWAYS;
	if (!uinfo) return;
	const char *u = getuser(uid);
	const char *g = getgroup(gid);
	putc(' ', out);
	fputs(C_USERINFO, out);
	fputs(u, out);
	int uw = strlen(u);
	for (int n = uwidth - uw; n; n--)
		putc(' ', out);
	putc(' ', out);
	fputs(g, out);
	int gw = strlen(g);
	for (int n = gwidth - gw; n; n--)
		putc(' ', out);
}

static void fmt_file(FILE *out, time_t now, struct file_info *f) {
	fmt_strmode(out, f->mode);
	fmt_user(out, f->uid, f->gid);
	if (options.longdate)
		fmt_longtime(out, now, f->time);
	else
		fmt_reltime(out, now, f->time);
	putc(' ', out);
	fmt_size(out, f->size);
	putc(' ', out);
	fmt_name(out, f);
	putc('\n', out);
}

void usage(void) {
	log("Usage: %s [option ...] [file ...]"
		"\n  -F       append file type indicator"
		"\n  -a       show all files"
		"\n  -c       use ctime"
		"\n  -g       group directories first"
		"\n  -S       sort by size"
		"\n  -t       sort by time"
		"\n  -r       reverse sort"
		"\n  -D       long date format"
		"\n  -u       user info"
		"\n  -U       automatic user info"
		"\n  -h       show this help"
		, program_name);
}

int main(int argc, char **argv) {
	int c;
	while ((c = getopt(argc, argv, "aFcrgtShuUD")) != -1) {
		switch (c) {
		case 'F': options.classify = true; break;
		case 'a': options.all = true; break;
		case 'c': options.ctime = true; break;
		case 'r': options.reverse = true; break;
		case 'g': options.group_dir = true; break;
		case 'S': options.sort = SORT_SIZE; break;
		case 't': options.sort = SORT_TIME; break;
		case 'u': options.userinfo = UINFO_AUTO; break;
		case 'U': options.userinfo = UINFO_ALWAYS; break;
		case 'D': options.longdate = true; break;
		case 'h': usage(); exit(EXIT_SUCCESS); break;
		default: exit(EXIT_FAILURE); break;
		}
	}
	if (optind >= argc) argv[--optind] = ".";
	myuid = getuid(), mygid = getgid();
	lsc_parse(&ls_colors, getenv("LS_COLORS"));
	file_vec v;
	fv_init(&v, 64);
	time_t now = current_time();
	FILE *out = stdout;
	char buf[BUFSIZ];
	setvbuf(out, buf, _IOFBF, BUFSIZ);
	int err = EXIT_SUCCESS;
	while (optind < argc) {
		if (ls(&v, argv[optind++]) == -1)
			err = EXIT_FAILURE;
		fv_sort(&v, fi_cmp);
		if (options.userinfo == UINFO_ALWAYS || uinfo_auto)
			for (size_t i = 0; i < v.len; i++) {
				struct file_info *fi = fv_index(&v, i);
				int uw = strlen(getuser(fi->uid));
				int gw = strlen(getgroup(fi->gid));
				if (uw > uwidth) uwidth = uw;
				if (gw > gwidth) gwidth = gw;
			}
		for (size_t i = 0; i < v.len; i++) {
			struct file_info *fi = fv_index(&v, i);
			fmt_file(out, now, fi);
			fi_free(fi);
		}
		fv_clear(&v);
	};
	exit(err);
}
