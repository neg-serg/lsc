#define _DEFAULT_SOURCE

#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

#include "conf.h"
#include "fbuf.h"
#include "modes.h"

void typeletter(fb_t *out, const mode_t mode);

inline void typeletter(fb_t *out, const mode_t mode) {
	switch (mode&S_IFMT) {
	// these are the most common, so test for them first.
	case S_IFREG:
		fb_ws(out, cNone);
		break;
	case S_IFDIR:
		fb_ws(out, cDir);
		break;
	// other letters standardized by POSIX 1003.1-2004.
	case S_IFCHR:
		fb_ws(out, cChar);
		break;
	case S_IFBLK:
		fb_ws(out, cBlock);
		break;
	case S_IFIFO:
		fb_ws(out, cFifo);
		break;
	case S_IFLNK:
		fb_ws(out, cLink);
		break;
	// other file types (though not letters) standardized by POSIX.
	case S_IFSOCK:
		fb_ws(out, cSock);
		break;
	default:
		fb_putc(out, '?');
		break;
	}
}

/*
func typeletterNoColor(mode_t mode) []byte {
	switch mode & S_IFMT {
	// these are the most common, so test for them first.
	case S_IFREG:
		return nNone
	case S_IFDIR:
		return nDir
	// other letters standardized by POSIX 1003.1-2004.
	case S_IFCHR:
		return nChar
	case S_IFBLK:
		return nBlock
	case S_IFIFO:
		return nFifo
	case S_IFLNK:
		return nLink
	// other file types (though not letters) standardized by POSIX.
	case S_IFSOCK:
		return nSock
	}
	return []byte("?")
}
*/

// create mode strings
inline void strmode(fb_t *out, const mode_t mode) {
	typeletter(out, mode);
	if ((mode&S_IRUSR) != 0) {
		fb_ws(out, cRead);
	} else {
		fb_ws(out, cNone);
	}

	if ((mode&S_IWUSR) != 0) {
		fb_ws(out, cWrite);
	} else {
		fb_ws(out, cNone);
	}

	if ((mode&S_ISUID) != 0) {
		if ((mode&S_IXUSR) != 0) {
			fb_ws(out, cUIDExec);
		} else {
			fb_ws(out, cUID);
		}
	} else if ((mode&S_IXUSR) != 0) {
		fb_ws(out, cExec);
	} else {
		fb_ws(out, cNone);
	}

	if ((mode&S_IRGRP) != 0) {
		fb_ws(out, cRead);
	} else {
		fb_ws(out, cNone);
	}

	if ((mode&S_IWGRP) != 0) {
		fb_ws(out, cWrite);
	} else {
		fb_ws(out, cNone);
	}

	if ((mode&S_ISGID) != 0) {
		if ((mode&S_IXGRP) != 0) {
			fb_ws(out, cUIDExec);
		} else {
			fb_ws(out, cUID);
		}
	} else if ((mode&S_IXGRP) != 0) {
		fb_ws(out, cExec);
	} else {
		fb_ws(out, cNone);
	}

	if ((mode&S_IROTH) != 0) {
		fb_ws(out, cRead);
	} else {
		fb_ws(out, cNone);
	}

	if ((mode&S_IWOTH) != 0) {
		fb_ws(out, cWrite);
	} else {
		fb_ws(out, cNone);
	}

	if ((mode&S_ISVTX) != 0) {
		if ((mode&S_IXOTH) != 0) {
			fb_ws(out, cSticky);
		} else {
			fb_ws(out, cStickyO);
		}
	} else if ((mode&S_IXOTH) != 0) {
		fb_ws(out, cExec);
	} else {
		fb_ws(out, cNone);
	}
}

/*
// create mode strings
void strmode_no_color(FILE *out, mode_t mode) {
	fputs(typeletterNoColor(mode), out)
	if mode&S_IRUSR != 0 {
		fputs(nRead, out)
	} else {
		fputs(nNone, out)
	}

	if mode&S_IWUSR != 0 {
		fputs(nWrite, out)
	} else {
		fputs(nNone, out)
	}

	if mode&S_ISUID != 0 {
		if mode&S_IXUSR != 0 {
			fputs(nUIDExec, out)
		} else {
			fputs(nUID, out)
		}
	} else if mode&S_IXUSR != 0 {
		fputs(nExec, out)
	} else {
		fputs(nNone, out)
	}

	if mode&S_IRGRP != 0 {
		fputs(nRead, out)
	} else {
		fputs(nNone, out)
	}

	if mode&S_IWGRP != 0 {
		fputs(nWrite, out)
	} else {
		fputs(nNone, out)
	}

	if mode&S_ISGID != 0 {
		if mode&S_IXGRP != 0 {
			fputs(nUIDExec, out)
		} else {
			fputs(nUID, out)
		}
	} else if mode&S_IXGRP != 0 {
		fputs(nExec, out)
	} else {
		fputs(nNone, out)
	}

	if mode&S_IROTH != 0 {
		fputs(nRead, out)
	} else {
		fputs(nNone, out)
	}

	if mode&S_IWOTH != 0 {
		fputs(nWrite, out)
	} else {
		fputs(nNone, out)
	}

	if mode&S_ISVTX != 0 {
		if mode&S_IXOTH != 0 {
			fputs(nSticky, out)
		} else {
			fputs(nStickyO, out)
		}
	} else if mode&S_IXOTH != 0 {
		fputs(nExec, out)
	} else {
		fputs(nNone, out)
	}
}
*/
