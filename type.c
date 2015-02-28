#define _DEFAULT_SOURCE

#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

#include "fbuf.h"
#include "modes.h"
#include "type.h"

inline enum indicator color_type(mode_t mode) {
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
