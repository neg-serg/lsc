#define _DEFAULT_SOURCE

#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "fbuf.h"
#include "filevercmp.h"
#include "time.h"
#include "util.h"
#include "stat.h"
#include "args.h"

uint32_t gettime(struct stat *st);

void file_list_init(struct file_list *fl) {
	fl->data = xmalloc(sizeof(struct file_info)*128);
	fl->cap = 128;
	fl->len = 0;
	fl->malloc = false;
}

void file_list_clear(struct file_list *fl) {
	if (fl->malloc) {
		for (size_t i = 0; i < fl->len; i++) {
			free((char*)fl->data[i].name.str);
			free((char*)fl->data[i].linkname.str);
		}
	}
	fl->len = 0;
}

void file_list_free(struct file_list *fl) {
	free(fl->data);
	fl->cap = 0;
	fl->len = 0;
}

inline uint32_t gettime(struct stat *st) {
	if (opts.ctime) {
		return ts_to_u32(st->st_ctim);
	}
	return ts_to_u32(st->st_mtim);
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

static const char *ls_readlink(char *name, size_t size) {
	char *buf = xmalloc(size+1); // allocate length + \0
	ssize_t n = readlink(name, buf, size);
	if (n == -1) {
		free(buf);
		return NULL;
	}
	assert((size_t)n == size); // XXX: symlink grew, handle this better
	buf[size] = '\0';
	return buf;
}

// stat writes a file_info describing the named file
static int ls_stat(char *path, char *name, struct file_info *out) {
	struct stat st;
	if (lstat(path, &st) == -1) {
		return -1;
	}

	*out = (struct file_info) {
		.name = new_suf_indexed(name),
		.size = (size_t)st.st_size,
		.mode = st.st_mode,
		.time = gettime(&st),
		.linkok = true,
		.linkname = {NULL, 0, 0},
		.linkmode = 0,
	};

	if (S_ISLNK(out->mode)) {
		const char *ln = ls_readlink(path, (size_t)st.st_size);
		if (ln == NULL) {
			out->linkok = false;
			return 0;
		}
		out->linkname = new_suf_indexed(ln);
		if (stat(path, &st) == -1) {
			out->linkok = false;
			return 0;
		}
		out->linkmode = st.st_mode;
	}

	return 0;
}

static int ls_readdir(struct file_list *l, char *name) {
	DIR *dir = opendir(name);
	if (dir == NULL) {
		return -1;
	}

	long name_max = pathconf(name, _PC_NAME_MAX);
	if (name_max == -1) {
		name_max = 255; // take a guess
	}

	struct dirent *dent;
	size_t dname_len = strlen(name);

	char *fpath = xmalloc(dname_len + 1 + name_max);
	char *fpathw = fpath+dname_len+1;
	memcpy(fpath, name, dname_len);
	fpath[dname_len] = '/';

	while ((dent = readdir(dir)) != NULL) {
		char *dn = dent->d_name;
		if ((dn[0] == '.') &&
			((!opts.all) || (dn[1] == '\0') ||
			 (dn[1] == '.' && dn[2] == '\0'))) {
			continue;
		}
		if (l->len >= l->cap) {
			l->cap *= 2;
			l->data = xrealloc(l->data, sizeof(struct file_info)*l->cap);
		}
		strncpy(fpathw, dn, name_max);
		if (ls_stat(fpath, strdup(dn), l->data+l->len) == -1) {
			free(fpath);
			closedir(dir);
			return -1;
		}
		l->len++;
	}
	free(fpath);
	return closedir(dir);
}

// get info about file/directory name
// bufsize must be at least 1
void ls(struct file_list *l, char *name) {
	if (ls_stat(name, name, l->data) == -1) {
		die_errno();
	}
	if (S_ISDIR(l->data->mode)) {
		if (ls_readdir(l, name) == -1) {
			die_errno();
		}
		l->malloc = true;
		return;
	}
	l->malloc = false;
	l->len = 1;
}
