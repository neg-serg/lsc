#define _GNU_SOURCE

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "xxhash/xxhash.h"

#define BUFLEN 65536

#include "filevercmp.h"
#include "ht.h"
#include "util.h"
#include "fbuf.h"
#include "config.h"

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

void usage(void) {
	logf("Usage: %s [option ...] [file ...]\n"
		"  -C when  use colours (never, always or auto)\n"
		"  -F       append file type indicator\n"
		"  -a       show all files\n"
		"  -c       use ctime\n"
		"  -r       reverse sort\n"
		"  -g       group directories first\n"
		"  -S       sort by size\n"
		"  -t       sort by time\n"
		"  -h       show this help",
		program_invocation_name);
}

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

enum use_color {
	COLOR_NEVER,
	COLOR_ALWAYS,
	COLOR_AUTO,
};

enum sort_type {
	SORT_SIZE,
	SORT_FVER,
	SORT_TIME,
};

struct opts {
	char **rest;
	size_t restc;
	enum sort_type sort;
	enum use_color color;
	int reverse;
	bool all;
	bool classify;
	bool ctime;
	bool group_dir;
};

static struct opts opts;

//
// Sorter
//

static int sorter(const struct file_info *a, const struct file_info *b)
{
	if (opts.group_dir) {
		if (S_ISDIR(a->linkmode) != S_ISDIR(b->linkmode)) {
			return S_ISDIR(a->linkmode) ? -1 : 1;
		}
		if (S_ISDIR(a->mode) != S_ISDIR(b->mode)) {
			return S_ISDIR(a->mode) ? -1 : 1;
		}
	}
	if (opts.sort == SORT_SIZE) {
		register ssize_t s = a->size-b->size;
		if (s) { return ((s>0)-(s<0))*opts.reverse; }
	} else if (opts.sort == SORT_TIME) {
		register time_t t = a->time - b->time;
		if (t) { return ((t>0)-(t<0))*opts.reverse; }
	}
	return filevercmp(a->name, b->name)*opts.reverse;
}

#define SORT_NAME fi
#define SORT_TYPE struct file_info
#define SORT_CMP(x, y) (sorter(&x, &y))
#include "sort/sort.h"

//
// Argument parsing
//

static int parse_arg_colorize(int argc, char **argv, char **s, int i) {
	char *use;
	if ((*s)[1] == '\0') {
		if (i+1 < argc) {
			// No argument
			warn("option '-C' needs an argument\n");
			usage();
			die();
		}
		// Argument is next one
		use = argv[i+1];
		i++;
	} else {
		// Argument is part of this one
		use = *s+1;
		*s += strlen(*s)-1;
	}

	if (strcmp("never", use) == 0) {
		opts.color = COLOR_NEVER;
	} else if (strcmp("always", use) == 0) {
		opts.color = COLOR_ALWAYS;
	} else if (strcmp("auto", use) == 0) {
		opts.color = COLOR_AUTO;
	} else {
		warnf("invalid argument to option '-C': \"%s\"", use);
		usage();
		die();
	}
	return i;
}

static void parse_args(int argc, char **argv)
{
	opts.color = COLOR_AUTO;
	opts.sort = SORT_FVER;
	opts.rest = xmallocr((size_t)argc, sizeof(char *));
	opts.reverse = 1;
	size_t restc = 0;

	for (int i = 1; i < argc; i++) {
		char* s = argv[i];
		if (s[0] == '\0' || s[0] != '-' || s[1] == '\0') {
			// not an option
			opts.rest[restc] = s;
			restc++;
			continue;
		}
		if (s[1] == '-' && s[2] == '\0') {
			// "--" ends opts
			for (i = i+1; i < argc; i++) {
				opts.rest[restc] = argv[i];
				restc++;
			}
			break;
		}
		// loop through opts
		s++;
		for (char f = *s; f != '\0'; f = *(++s)) {
			switch (f) {
			case 'a':
				opts.all = true;
				break;
			case 'F':
				opts.classify = true;
				break;
			case 'c':
				opts.ctime = true;
				break;
			case 'r':
				opts.reverse = -1;
				break;
			case 'g':
				opts.group_dir = true;
				break;
			case 't':
				opts.sort = SORT_TIME;
				break;
			case 'S':
				opts.sort = SORT_SIZE;
				break;
			case 'C':
				i = parse_arg_colorize(argc, argv, &s, i);
				break;
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			default:
				warnf("unsupported option '%c'", f);
				usage();
				die();
			}
		}
	}
	if (restc == 0) {
		opts.rest[0] = ".";
		restc = 1;
	}
	opts.restc = restc;
}

//
// File listing
//

static void file_list_init(struct file_list *fl)
{
	fl->data = xmallocr(128, sizeof(struct file_info));
	fl->cap = 128;
	fl->len = 0;
}

static void file_list_clear(struct file_list *fl)
{
	for (size_t i = 0; i < fl->len; i++) {
		free((char*)fl->data[i].name.str);
		free((char*)fl->data[i].linkname.str);
	}
	fl->len = 0;
}

static void file_list_free(struct file_list *fl)
{
	free(fl->data);
	fl->cap = 0;
	fl->len = 0;
}

/*
char *clean_right(char *path) []byte {
	for i := len(path); i > 0; i-- {
		if path[i-1] != '/' {
			return path[:i]
		}
	}
	return path
}
*/

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
	assertx((size_t)n <= size); // XXX: symlink grew, handle this better
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

	*out = (struct file_info) {
		.name = new_suf_indexed(name),
		.linkname = (struct suf_indexed) {0},
		.mode = st.st_mode,
		.linkmode = 0,
		.time = opts.ctime ? st.st_ctim.tv_sec : st.st_mtim.tv_sec,
		.size = st.st_size,
		.linkok = true,
	};

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
		warnf("cannot open directory %s: %s", name, strerror(errno));
		return -1;
	}
	int fd = dirfd(dir);
	if (fd == -1) {
		warnf("%s: %s", name, strerror(errno));
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
			warnf("cannot access %s/%s: %s", name, dn, strerror(errno));
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
// bufsize must be at least 1
int ls(struct file_list *l, char *name)
{
	char *s = strdup(name); // make freeing simpler
	if (ls_stat(AT_FDCWD, s, l->data) == -1) {
		warnf("cannot access %s: %s", s, strerror(errno));
		free(s);
		_exit(EXIT_FAILURE);
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

enum indicator {
	IND_LEFT,
	IND_RIGHT,
	IND_END,
	IND_RESET,
	IND_NORM,
	IND_FILE,
	IND_DIR,
	IND_LINK,
	IND_FIFO,
	IND_SOCK,
	IND_BLK,
	IND_CHR,
	IND_MISSING,
	IND_ORPHAN,
	IND_EXEC,
	IND_DOOR,
	IND_SETUID,
	IND_SETGID,
	IND_STICKY,
	IND_OTHERWRITABLE,
	IND_STICKYOTHERWRITABLE,
	IND_CAP,
	IND_MULTIHARDLINK,
	IND_CLRTOEOL,
};

struct ind_name {
	const char* name;
	enum indicator code;
};

// Derived from colorh.gperf
static const struct ind_name *ind_name_lookup(const char *str,
	const unsigned int len)
{
#define MAX_HASH_VALUE 53
#define WORD_LENGTH 2
	static const unsigned char asso_values[] = {
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 23, 30,
		10, 10, 13, 54, 11, 28,  3, 54, 18, 15,
		6,  5,  0,  54, 20,  0, 15, 1,  8,  54,
		10, 28, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
		54, 54, 54, 54, 54, 54, 54,
	};

	static const struct ind_name wordlist[] = {
		{"so", IND_SOCK},
		{"st", IND_STICKY},
		{NULL, 0},
		{"pi", IND_FIFO},
		{NULL, 0},
		{"or", IND_ORPHAN},
		{"no", IND_NORM},
		{NULL, 0},
		{"su", IND_SETUID},
		{NULL, 0},
		{"do", IND_DOOR},
		{"sg", IND_SETGID},
		{NULL, 0},
		{"di", IND_DIR},
		{NULL, 0},
		{"ow", IND_OTHERWRITABLE},
		{"fi", IND_FILE},
		{NULL, 0},
		{"mi", IND_MISSING},
		{NULL, 0},
		{"ec", IND_END},
		{NULL, 0}, {NULL, 0},
		{"ln", IND_LINK},
		{NULL, 0},
		{"tw", IND_STICKYOTHERWRITABLE},
		{NULL, 0}, {NULL, 0},
		{"lc", IND_LEFT},
		{NULL, 0},
		{"rc", IND_RIGHT},
		{NULL, 0}, {NULL, 0},
		{"bd", IND_BLK},
		{NULL, 0},
		{"rs", IND_RESET},
		{NULL, 0}, {NULL, 0},
		{"ex", IND_EXEC},
		{NULL, 0},
		{"cd", IND_CHR},
		{NULL, 0}, {NULL, 0},
		{"mh", IND_MULTIHARDLINK},
		{NULL, 0},
		{"cl", IND_CLRTOEOL},
		{NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0},
		{NULL, 0}, {NULL, 0}, {NULL, 0},
		{"ca", IND_CAP},
	};

	if (len == WORD_LENGTH) {
		const int key = asso_values[(unsigned char)str[1]+1] +
			asso_values[(unsigned char)str[0]];
		if (key <= MAX_HASH_VALUE && key >= 0) {
			const char *s = wordlist[key].name;
			// WORD_LENGTH is 2
			if (s != NULL && str[0] == s[0] && str[1] == s[1]) {
				return &wordlist[key];
			}
		}
	}
	return 0;
}

// indexed by enum indicator
// XXX: global state
static char *ls_color_types[] = {
	"\033[",  // "lc": Left of color sequence
	"m",      // "rc": Right of color sequence
	"",       // "ec": End color (replaces lc+no+rc)
	"0",      // "rs": Reset to ordinary colors
	"",       // "no": Normal
	"",       // "fi": File: default
	"01;34",  // "di": Directory: bright blue
	"01;36",  // "ln": Symlink: bright cyan
	"33",     // "pi": Pipe: yellow/brown
	"01;35",  // "so": Socket: bright magenta
	"01;33",  // "bd": Block device: bright yellow
	"01;33",  // "cd": Char device: bright yellow
	"",       // "mi": Missing file: undefined
	"",       // "or": Orphaned symlink: undefined
	"01;32",  // "ex": Executable: bright green
	"01;35",  // "do": Door: bright magenta
	"37;41",  // "su": setuid: white on red
	"30;43",  // "sg": setgid: black on yellow
	"37;44",  // "st": sticky: black on blue
	"34;42",  // "ow": other-writable: blue on green
	"30;42",  // "tw": ow w/ sticky: black on green
	"30;41",  // "ca": black on red
	"",       // "mh": disabled by default
	"\033[K", // "cl": clear to end of line
};

static int keyeq(const ssht_key_t a, const ssht_key_t b)
{
	return a.len == b.len && memcmp(a.key, b.key, a.len) == 0;
}

static unsigned keyhash(const ssht_key_t a)
{
	return XXH32(a.key, a.len, 0);
}

// XXX: global state
static bool color_sym_target = false;
static char *lsc_env;
static ssht_t *ht;

static void parse_ls_color(void)
{
	lsc_env = getenv("LS_COLORS");
	ht = ssht_alloc(keyhash, keyeq);
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
			k.len = ke-kb-1;
			k.key = lsc_env+kb+1;

			lsc_env[i] = '\0';
			v = lsc_env+ke+1;

			ssht_set(ht, k, v);
		} else {
			// TODO make sure it's 2 long
			const char k[] = {lsc_env[kb], lsc_env[kb+1]};
			const struct ind_name *out =
				ind_name_lookup((const char*)&k, 2);
			//XXX handle error better
			assertx(out != NULL);
			lsc_env[i] = '\0';
			ls_color_types[out->code] = lsc_env+ke+1;
		}
		kb = i + 1;
		i += 2;
		eq = false;
	}
	if (strcmp(ls_color_types[IND_LINK], "target") == 0) {
		color_sym_target = true;
	}
}

static const char *color(char *name, size_t len, enum indicator in)
{
	if (in == IND_FILE || in == IND_LINK) {
		char *n = memrchr(name, '.', len);
		if (n != NULL) {
			ssht_key_t k;
			k.key = n;
			k.len = name+len-n;
			char *ret = ssht_get(ht, k);
			if (ret != NULL) { return ret; }
		}
	}
	if (in == IND_LINK) {
		return cSym;
	}
	return ls_color_types[in];
}

static enum indicator color_type(mode_t mode)
{
#define S_IXUGO (S_IXUSR|S_IXGRP|S_IXOTH)
	switch (mode&S_IFMT) {
	case S_IFREG:
		if ((mode&S_ISUID) != 0) {
			return IND_SETUID;
		} else if ((mode&S_ISGID) != 0) {
			return IND_SETGID;
		} else if ((mode&S_IXUGO) != 0) {
			return IND_EXEC;
		}
		return IND_FILE;
	case S_IFDIR:
		if ((mode&S_ISVTX) != 0 && (mode&S_IWOTH) != 0) {
			return IND_STICKYOTHERWRITABLE;
		} else if ((mode&S_IWOTH) != 0) {
			return IND_OTHERWRITABLE;
		} else if ((mode&S_ISVTX) != 0) {
			return IND_STICKY;
		}
		return IND_DIR;
	case S_IFLNK:
		return IND_LINK;
	case S_IFIFO:
		return IND_FIFO;
	case S_IFSOCK:
		return IND_SOCK;
	case S_IFCHR:
		return IND_CHR;
	case S_IFBLK:
		return IND_BLK;
	default:
		// anything else is classified as orphan
		return IND_ORPHAN;
	}
}

//
// Time printer
//

#define second 1
#define minute (60 * second)
#define hour   (60 * minute)
#define day    (24 * hour)
#define week   (7  * day)
#define month  (30 * day)
#define year   (12 * month)

static time_t current_time(void)
{
	struct timespec t;
	if (clock_gettime(CLOCK_REALTIME, &t) == -1) {
		die_errno();
	}
	return t.tv_sec;
}

void fmt3(char b[3], uint16_t x) {
	if (x/100) b[0] = '0' + x/100;
	if (x/100||x/10%10) b[1] = '0' + x/10%10;
	b[2] = '0' + x%10;
}

static void reltime_color(fb *out, const time_t now, const time_t then)
{
	char b[3] = "   ";
	const time_t diff = now - then;
	if (diff < 0) {
		fb_ws(out, cSecond " <0s" cEnd);
	} else if (diff <= second) {
		fb_ws(out, cSecond " <1s" cEnd);
	} else if (diff < minute) {
		fb_ws(out, cSecond);
		fmt3(b, diff);
		fb_write(out, b, 3);
		fb_ws(out, "s" cEnd);
	} else if (diff < hour) {
		fb_ws(out, cMinute);
		fmt3(b, diff/minute);
		fb_write(out, b, 3);
		fb_ws(out, "m" cEnd);
	} else if (diff < hour*36) {
		fb_ws(out, cHour);
		fmt3(b, diff/hour);
		fb_write(out, b, 3);
		fb_ws(out, "h" cEnd);
	} else if (diff < month) {
		fb_ws(out, cDay);
		fmt3(b, diff/day);
		fb_write(out, b, 3);
		fb_ws(out, "d" cEnd);
	} else if (diff < year) {
		fb_ws(out, cWeek);
		fmt3(b, diff/week);
		fb_write(out, b, 3);
		fb_ws(out, "w" cEnd);
	} else {
		fb_ws(out, cYear);
		fmt3(b, diff/year);
		fb_write(out, b, 3);
		fb_ws(out, "y" cEnd);
	}
}

static void reltime_no_color(fb *out, const time_t now, const time_t then)
{
	const time_t diff = now - then;
	if (diff < 0) {
		fb_ws(out, " <0s");
	} else if (diff <= second) {
		fb_ws(out, " <1s" cEnd);
	} else if (diff < minute) {
		fb_u(out, diff, 3, ' ');
		fb_putc(out, 's');
	} else if (diff < hour) {
		fb_u(out, diff/minute, 3, ' ');
		fb_putc(out, 'm');
	} else if (diff < hour*36) {
		fb_u(out, diff/hour, 3, ' ');
		fb_putc(out, 'h');
	} else if (diff < month) {
		fb_u(out, diff/day, 3, ' ');
		fb_putc(out, 'd');
	} else if (diff < year) {
		fb_u(out, diff/week, 3, ' ');
		fb_putc(out, 'w');
	} else {
		fb_u(out, diff/year, 3, ' ');
		fb_putc(out, 'y');
	}
}

//
// Mode printer
//

#define write_tc(out, ts, i) fb_sstr((out), (ts)[(i)])

static void typeletter(fb *out, const mode_t mode, struct sstr ts[14])
{
	switch (mode&S_IFMT) {
	case S_IFREG:
		write_tc(out, ts, i_none);
		break;
	case S_IFDIR:
		write_tc(out, ts, i_dir);
		break;
	case S_IFCHR:
		write_tc(out, ts, i_char);
		break;
	case S_IFBLK:
		write_tc(out, ts, i_block);
		break;
	case S_IFIFO:
		write_tc(out, ts, i_fifo);
		break;
	case S_IFLNK:
		write_tc(out, ts, i_link);
		break;
	case S_IFSOCK:
		write_tc(out, ts, i_sock);
		break;
	default:
		write_tc(out, ts, i_unknown);
		break;
	}
}

// create mode strings
static void strmode(fb *out, const mode_t mode, struct sstr ts[14])
{
	typeletter(out, mode, ts);
	if (mode&S_IRUSR) {
		write_tc(out, ts, i_read);
	} else {
		write_tc(out, ts, i_none);
	}

	if (mode&S_IWUSR) {
		write_tc(out, ts, i_write);
	} else {
		write_tc(out, ts, i_none);
	}

	if (mode&S_ISUID) {
		if (mode&S_IXUSR) {
			write_tc(out, ts, i_uid_exec);
		} else {
			write_tc(out, ts, i_uid);
		}
	} else if (mode&S_IXUSR) {
		write_tc(out, ts, i_exec);
	} else {
		write_tc(out, ts, i_none);
	}

	if (mode&S_IRGRP) {
		write_tc(out, ts, i_read);
	} else {
		write_tc(out, ts, i_none);
	}

	if (mode&S_IWGRP) {
		write_tc(out, ts, i_write);
	} else {
		write_tc(out, ts, i_none);
	}

	if (mode&S_ISGID) {
		if (mode&S_IXGRP) {
			write_tc(out, ts, i_uid_exec);
		} else {
			write_tc(out, ts, i_uid);
		}
	} else if (mode&S_IXGRP) {
		write_tc(out, ts, i_exec);
	} else {
		write_tc(out, ts, i_none);
	}

	if (mode&S_IROTH) {
		write_tc(out, ts, i_read);
	} else {
		write_tc(out, ts, i_none);
	}

	if (mode&S_IWOTH) {
		write_tc(out, ts, i_write);
	} else {
		write_tc(out, ts, i_none);
	}

	if (mode&S_ISVTX) {
		if (mode&S_IXOTH) {
			write_tc(out, ts, i_sticky);
		} else {
			write_tc(out, ts, i_sticky_o);
		}
	} else if (mode&S_IXOTH) {
		write_tc(out, ts, i_exec);
	} else {
		write_tc(out, ts, i_none);
	}
}

//
// Size printer
//

static off_t divide(off_t x, off_t d) {
	return (x+((d-1)/2))/d;
}

static void write_size(fb *out, off_t sz, const struct sstr sufs[7])
{
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
	fb_write(out, b, 3);
	fb_sstr(out, sufs[m]);
}

static void size_color(fb *out, off_t size)
{
	fb_ws(out, cSize);
	write_size(out, size, cSizes);
}

static void size_no_color(fb *out, off_t size)
{
	write_size(out, size, nSizes);
}

//
// Name printer
//

static void name_color(fb *out, const struct file_info *f)
{
	enum indicator t;
	if (f->linkname.str != NULL && *f->linkname.str != '\0') {
		if (!f->linkok) {
			t = IND_ORPHAN;
		} else if (color_sym_target) {
			t = color_type(f->linkmode);
		} else {
			t = color_type(f->mode);
		}
	} else {
		t = color_type(f->mode);
	}
	const char *c = color((char*)f->name.str, f->name.len, t);
	if (c == NULL) {
		fb_write(out, f->name.str, f->name.len);
	} else {
		fb_ws(out, cESC);
		fb_puts(out, c);
		fb_putc(out, 'm');
		fb_write(out, f->name.str, f->name.len);
		fb_ws(out, cEnd);
	}
	if (f->linkname.str != NULL) {
		enum indicator lnt;
		if (!f->linkok) {
			lnt = IND_MISSING;
		} else {
			lnt = color_type(f->linkmode);
		}
		const char *lc = color((char*)f->linkname.str,
			f->linkname.len, lnt);
		if (lc == NULL || *lc == '\0') {
			fb_ws(out, cSymDelim cEnd);
			fb_write(out, f->linkname.str, f->linkname.len);
		} else {
			fb_ws(out, cSymDelim cESC);
			fb_puts(out, lc);
			fb_putc(out, 'm');
			fb_write(out, f->linkname.str, f->linkname.len);
			fb_ws(out, cEnd);
		}
	}
	if (opts.classify) {
		switch (t) {
		case IND_DIR:
			fb_putc(out, '/');
			break;
		case IND_EXEC:
			fb_putc(out, '*');
			break;
		case IND_FIFO:
			fb_putc(out, '|');
			break;
		case IND_SOCK:
			fb_putc(out, '=');
			break;
		}
	}
}

static void name_no_color(fb *out, const struct file_info *f)
{
	enum indicator t;
	if (f->linkname.str != NULL && *f->linkname.str != '\0') {
		if (!f->linkok) {
			t = IND_ORPHAN;
		} else if (color_sym_target) {
			t = color_type(f->linkmode);
		} else {
			t = color_type(f->mode);
		}
	} else {
		t = color_type(f->mode);
	}
	fb_write(out, f->name.str, f->name.len);
	if (f->linkname.str != NULL) {
		fb_ws(out, nSymDelim);
		fb_write(out, f->linkname.str, f->linkname.len);
	}
	if (opts.classify) {
		switch (t) {
		case IND_DIR:
			fb_putc(out, '/');
			break;
		case IND_EXEC:
			fb_putc(out, '*');
			break;
		case IND_FIFO:
			fb_putc(out, '|');
			break;
		case IND_SOCK:
			fb_putc(out, '=');
			break;
		}
	}
}

int main(int argc, char **argv)
{
	parse_args(argc, argv);
	parse_ls_color();
	bool colorize =
		(opts.color == COLOR_AUTO && isatty(STDOUT_FILENO)) ||
		opts.color == COLOR_ALWAYS;
	struct file_list l;
	file_list_init(&l);
	fb out;
	fb_init(&out, STDOUT_FILENO, BUFLEN);
	time_t now = current_time();
	int err = EXIT_SUCCESS;

	struct sstr *modes;
	void (*reltime)(fb *, const time_t, const time_t);
	void (*size)(fb *, off_t);
	void (*name)(fb *, const struct file_info *);

	if (colorize) {
		modes = cModes;
		reltime = reltime_color;
		size = size_color;
		name = name_color;
	} else {
		modes = nModes;
		reltime = reltime_no_color;
		size = size_no_color;
		name = name_no_color;
	}

	for (size_t i = 0; i < opts.restc; i++) {
		if (ls(&l, opts.rest[i]) == -1)
			err = EXIT_FAILURE;
		fi_tim_sort(l.data, l.len);
		for (size_t j = 0; j < l.len; j++) {
			strmode(&out, l.data[j].mode, modes);
			reltime(&out, now, l.data[j].time);
			fb_putc(&out, ' ');
			size(&out, l.data[j].size);
			fb_putc(&out, ' ');
			name(&out, &l.data[j]);
			fb_putc(&out, '\n');
		}
		file_list_clear(&l);
	};
	fb_flush(&out);
	file_list_free(&l);
	//fb_free(&out);
	return err;
}
