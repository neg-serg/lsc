#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "xxhash/xxhash.h"

#define program_name "lsc"

#include "slice.h"
#include "filevercmp.h"
#include "ht.h"
#include "util.h"
#include "config.h"

void usage(void) {
	log("usage: %s [option ...] [file ...]"
		"\n  -F       append file type indicator"
		"\n  -a       show all files"
		"\n  -c       use ctime"
		"\n  -g       group directories first"
		"\n  -S       sort by size"
		"\n  -t       sort by time"
		"\n  -r       reverse sort"
		"\n  -h       show this help"
		, program_name);
}

enum type_str_index {
	i_none,     // Nothing else applies

	i_read,     // Readable
	i_write,    // Writeable
	i_exec,     // Executable

	i_dir,      // Directory
	i_char,     // Character device
	i_block,    // Block device
	i_fifo,     // FIFO
	i_link,     // Symlink

	i_sock,     // Socket
	i_uid,      // SUID
	i_uid_exec, // SUID and executable
	i_sticky,   // Sticky
	i_sticky_o, // Sticky, writeable by others

	i_unknown,  // Anything else
};

enum file_kind {
	T_FILE,
	T_DIR,
	T_LINK,
	T_FIFO,
	T_SOCK,
	T_BLK,
	T_CHR,
	T_ORPHAN,
	T_EXEC,
	T_SETUID,
	T_SETGID,
	T_STICKY,
	T_OW,
	T_STICKYOW,
};

struct file_info {
	struct suf_indexed name;
	struct suf_indexed linkname;
	mode_t mode;
	mode_t linkmode;
	time_t time;
	off_t size;
	bool linkok;
};

struct file_list {
	struct file_info *data;
	size_t len;
	size_t cap;
};

enum sort_type {
	SORT_FVER,
	SORT_SIZE,
	SORT_TIME,
};

struct opts {
	enum sort_type sort;
	bool all;
	bool classify;
	bool ctime;
	bool group_dir;
	bool reverse;
};

static struct opts opts;

//
// Sorter
//

static int sorter(const void *va, const void *vb) {
	struct file_info *a = *(struct file_info *const *)va;
	struct file_info *b = *(struct file_info *const *)vb;
	int rev = opts.reverse ? -1 : 1;
	if (opts.group_dir) {
		if (S_ISDIR(a->linkmode) != S_ISDIR(b->linkmode))
			return S_ISDIR(a->linkmode) ? -1 : 1;
		if (S_ISDIR(a->mode) != S_ISDIR(b->mode))
			return S_ISDIR(a->mode) ? -1 : 1;
	}
	if (opts.sort == SORT_SIZE) {
		ssize_t s = a->size-b->size;
		if (s) return ((s>0)-(s<0))*rev;
	} else if (opts.sort == SORT_TIME) {
		time_t t = a->time - b->time;
		if (t) return ((t>0)-(t<0))*rev;
	}
	return filevercmp(a->name, b->name)*rev;
}

static void fi_sort(struct file_info **l, size_t len) {
	qsort(l, len, sizeof(struct file_info *), sorter);
}

//
// File listing
//

static void fi_init(struct file_list *fl)
{
	fl->data = xmallocr(128, sizeof(struct file_info));
	fl->cap = 128;
	fl->len = 0;
}

static void fi_clear(struct file_list *fl)
{
	fl->len = 0;
}

// guarantees that returned string is size long
static int ls_readlink(int dirfd, char *name, size_t size,
	struct suf_indexed *out)
{
	char *buf = xmalloc(size+1); // allocate length + \0
	ssize_t n = readlinkat(dirfd, name, buf, size);
	if (n == -1) {
		free(buf);
		return -1;
	}
	assertx((size_t)n <= size); // XXX: symlink grew, handle this properly
	buf[n] = '\0';
	*out = new_suf_indexed_len(buf, n);
	return 0;
}

// stat writes a file_info describing the named file
static int ls_stat(int dirfd, char *name, struct file_info *out)
{
	struct stat st;
	if (fstatat(dirfd, name, &st, AT_SYMLINK_NOFOLLOW) == -1) {
		return -1;
	}

	out->name = new_suf_indexed(name);
	out->linkname = (struct suf_indexed) {0};
	out->mode = st.st_mode;
	out->linkmode = 0;
	out->time = opts.ctime ? st.st_ctim.tv_sec : st.st_mtim.tv_sec;
	out->size = st.st_size;
	out->linkok = true;

	if (S_ISLNK(out->mode)) {
		int ln = ls_readlink(dirfd, name,
			st.st_size,
			&(out->linkname));
		if (ln == -1) {
			out->linkok = false;
			return 0;
		}
		if (fstatat(dirfd, name, &st, 0) == -1) {
			out->linkok = false;
			return 0;
		}
		out->linkmode = st.st_mode;
	}

	return 0;
}

static int ls_readdir(struct file_list *l, char *name)
{
	for (size_t l = strlen(name)-1; name[l] == '/' && l>1; l--) {
		name[l] = '\0';
	}
	int err = 0;
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
	while ((dent = readdir(dir)) != NULL) {
		char *dn = dent->d_name;
		// skip ".\0", "..\0" and
		// names starting with '.' when opts.all is true
		if (dn[0] == '.' &&
			(!opts.all || dn[1] == '\0' ||
			(dn[1] == '.' && dn[2] == '\0'))) {
			continue;
		}
		if (l->len >= l->cap) {
			assertx(!size_mul_overflow(l->cap, 2, &l->cap));
			l->data = xreallocr(l->data, l->cap,
				sizeof(struct file_info));
		}
		dn = strdup(dn);
		if (ls_stat(fd, dn, l->data+l->len) == -1) {
			warn_errno("cannot access %s/%s", name, dn);
			err = -1; // Return -1 on errors
			free(dn);
			continue;
		}
		l->len++;
	}
	if (closedir(dir) == -1) {
		return -1;
	}
	return err;
}

// get info about file/directory name
static int ls(struct file_list *l, char *name)
{
	char *s = strdup(name);
	if (ls_stat(AT_FDCWD, s, l->data) == -1) {
		warn_errno("cannot access %s", s);
		free(s);
		exit(EXIT_FAILURE);
	}
	if (S_ISDIR(l->data->mode)) {
		free(s);
		if (ls_readdir(l, name) == -1) {
			return -1;
		}
		return 0;
	}
	l->len = 1;
	return 0;
}

//
// LS_COLORS parser
//

static unsigned keyhash(const ssht_key_t a)
{
	return XXH32(a.buf, a.len, 0);
}

static char *lsc_env;
static ssht_t *ht;

static void parse_ls_color(void)
{
	lsc_env = getenv("LS_COLORS");
	ht = ssht_alloc(keyhash, buf_eq);
	if (!lsc_env) return;
	bool eq = false;
	size_t kb = 0, ke = 0;
	for (size_t i = 0; lsc_env[i] != '\0'; i++) {
		const char b = lsc_env[i];
		if (b == '=') {
			ke = i;
			eq = true;
			continue;
		}
		if (!eq || b != ':') {
			continue;
		}
		if (lsc_env[kb] == '*') {
			ssht_key_t k;
			ssht_value_t v;
			lsc_env[ke] = '\0';
			k = buf_new(lsc_env+kb+1, ke-kb-1);
			lsc_env[i] = '\0';
			v = lsc_env+ke+1;
			ssht_set(ht, k, v);
		}
		kb = i + 1;
		i += 2;
		eq = false;
	}
}

static enum file_kind color_type(mode_t mode)
{
#define S_IXUGO (S_IXUSR|S_IXGRP|S_IXOTH)
	switch (mode&S_IFMT) {
	case S_IFREG:
		if ((mode&S_ISUID) != 0)
			return T_SETUID;
		if ((mode&S_ISGID) != 0)
			return T_SETGID;
		if ((mode&S_IXUGO) != 0)
			return T_EXEC;
		return T_FILE;
	case S_IFDIR:
		if ((mode&S_ISVTX) != 0 && (mode&S_IWOTH) != 0)
			return T_STICKYOW;
		if ((mode&S_IWOTH) != 0)
			return T_OW;
		if ((mode&S_ISVTX) != 0)
			return T_STICKY;
		return T_DIR;
	case S_IFLNK:  return T_LINK;
	case S_IFIFO:  return T_FIFO;
	case S_IFSOCK: return T_SOCK;
	case S_IFCHR:  return T_CHR;
	case S_IFBLK:  return T_BLK;
	// anything else is classified as orphan
	default: return T_ORPHAN;
	}
}

//
// Time printer
//

#define SECOND 1
#define MINUTE (60 * SECOND)
#define HOUR   (60 * MINUTE)
#define DAY    (24 * HOUR)
#define WEEK   (7  * DAY)
#define MONTH  (30 * DAY)
#define YEAR   (12 * MONTH)

static time_t current_time(void)
{
	struct timespec t;
	if (clock_gettime(CLOCK_REALTIME, &t) == -1) {
		die_errno("%s", "current_time");
	}
	return t.tv_sec;
}

static void fmt3(char b[3], uint16_t x)
{
	if (x/100) b[0] = '0' + x/100;
	if (x/100||x/10%10) b[1] = '0' + x/10%10;
	b[2] = '0' + x%10;
}

static void reltime(FILE *out, const time_t now, const time_t then)
{
	time_t diff = now - then;
	char b[4] = "  0s";

	if (diff < 0) {
		fputs(C_SECOND "  0s" C_END, out);
		return;
	}

	if (diff <= SECOND) {
		fputs(C_SECOND " <1s" C_END, out);
		return;
	}

	if (diff < MINUTE) {
		fputs(C_SECOND, out);
		b[3] = 's';
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

//
// Mode printer
//

// create mode strings
static void strmode(FILE *out, const mode_t mode, const char **ts)
{
	#define tc(out, s) fputs(ts[(s)], (out))
	switch (mode&S_IFMT) {
	case S_IFREG: tc(out, i_none); break;
	case S_IFDIR: tc(out, i_dir); break;
	case S_IFCHR: tc(out, i_char); break;
	case S_IFBLK: tc(out, i_block); break;
	case S_IFIFO: tc(out, i_fifo); break;
	case S_IFLNK: tc(out, i_link); break;
	case S_IFSOCK: tc(out, i_sock); break;
	default: tc(out, i_unknown); break;
	}
	tc(out, mode&S_IRUSR ? i_read : i_none);
	tc(out, mode&S_IWUSR ? i_write : i_none);
	tc(out, mode&S_ISUID
		? mode&S_IXUSR ? i_uid_exec : i_uid
		: mode&S_IXUSR ? i_exec : i_none);
	tc(out, mode&S_IRGRP ? i_read : i_none);
	tc(out, mode&S_IWGRP ? i_write : i_none);
	tc(out, mode&S_ISGID
		? mode&S_IXGRP ? i_uid_exec : i_uid
		: mode&S_IXGRP ? i_exec : i_none);
	tc(out, mode&S_IROTH ? i_read : i_none);
	tc(out, mode&S_IWOTH ? i_write : i_none);
	tc(out, mode&S_ISVTX
		? mode&S_IXOTH ? i_sticky : i_sticky_o
		: mode&S_IXOTH ? i_exec : i_none);
	#undef tc
}


//
// Size printer
//

static off_t divide(off_t x, off_t d)
{
	return (x+((d-1)/2))/d;
}

static void size(FILE *out, off_t sz, const char **sufs)
{
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
	char b[3] = "  0";
	if (v/10 >= 10 || m == 0) {
		fmt3(b, u);
	} else if (sz != 0) {
		b[0] = '0' + v/10;
		b[1] = '.';
		b[2] ='0' + v%10;
	}
	fwrite(b, 1, 3, out);
	fputs(sufs[m], out);
}

//
// Name printer
//

static const char *suf_color(buf name)
{
	char *n = memrchr(name.buf, '.', name.len);
	if (n != NULL)
		return ssht_get(ht, slice(name, n - name.buf, name.len));
	return NULL;
}

static const char *file_color(buf name, enum file_kind t) {
	const char *c;
	if (t == T_FILE || t == T_LINK) {
		c = suf_color(name);
		if (c != NULL) {
			return c;
		}
	}
	return c_kinds[t];
}

static void name(FILE *out, const struct file_info *f)
{
	enum file_kind t;
	const char *c;
	if (f->linkname.b.buf) {
		if (f->linkok) {
			t = color_type(f->linkmode);
		} else {
			t = T_ORPHAN;
		}
		c = file_color(f->linkname.b, t);
	} else {
		t = color_type(f->mode);
		c = file_color(f->name.b, t);
	}
	fputs(C_ESC, out);
	fputs(c, out);
	fputs("m", out);
	fwrite(f->name.b.buf, 1, f->name.b.len, out);
	fputs(C_END, out);
	if (f->linkname.b.buf) {
		fputs(C_SYM_DELIM C_ESC, out);
		fputs(c, out);
		fputs("m", out);
		fwrite(f->linkname.b.buf, 1, f->linkname.b.len, out);
		fputs(C_END, out);
	}
	if (opts.classify)
		switch (t) {
		case T_DIR:  fputs(CL_DIR, out); break;
		case T_EXEC: fputs(CL_EXEC, out); break;
		case T_FIFO: fputs(CL_FIFO, out); break;
		case T_SOCK: fputs(CL_SOCK, out); break;
		default: ; // shut up
		}
}

// uuugh
int main(int argc, char **argv)
{
	int c;
	while ((c = getopt(argc, argv, "aFcrgtS:h")) != -1) {
		switch (c) {
		case 'F': opts.classify = true; break;
		case 'a': opts.all = true; break;
		case 'c': opts.ctime = true; break;
		case 'r': opts.reverse = true; break;
		case 'g': opts.group_dir = true; break;
		case 'S': opts.sort = SORT_SIZE; break;
		case 't': opts.sort = SORT_TIME; break;
		case 'h': usage(); exit(EXIT_SUCCESS); break;
		default: exit(EXIT_FAILURE); break;
		}
	}

	// list . if no args
	if (optind >= argc) argv[--optind] = ".";

	parse_ls_color();
	struct file_list l;
	fi_init(&l);
	time_t now = current_time();
	struct file_info **ll = NULL;
	size_t lllen = 0;
	struct file_info *fi;
	FILE *out = stdout;
	int err = EXIT_SUCCESS;

	while (optind < argc) {
		if (ls(&l, argv[optind++]) == -1)
			err = EXIT_FAILURE;
		if (l.len>lllen) {
			ll = xreallocr(ll, l.len, sizeof(*ll));
			lllen=l.len;
		}
		for (size_t j = 0; j < l.len; j++) {
			ll[j] = &l.data[j];
		}
		fi_sort(ll, l.len);
		for (size_t j = 0; j < l.len; j++) {
			fi = ll[j];
			strmode(out, fi->mode, c_modes);
			reltime(out, now, fi->time);
			putc(' ', out);
			size(out, fi->size, c_sizes);
			putc(' ', out);
			name(out, fi);
			putc('\n', out);
			free((char*)(fi->name.b.buf));
			free((char*)(fi->linkname.b.buf));
		}
		fi_clear(&l);
	};

	exit(err);
}
