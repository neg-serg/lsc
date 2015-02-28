#define _DEFAULT_SOURCE

#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "filevercmp.h"
#include "stat.h"
#include "util.h"

#include "args.h"

static inline int sorter(const struct file_info *a, const struct file_info *b) {
	bool ad = S_ISDIR(a->linkmode);
	if (ad != S_ISDIR(b->linkmode)) {
		return ad ? -1 : 1;
	}
	ad = S_ISDIR(a->mode);
	if (ad != S_ISDIR(b->mode)) {
		return ad ? -1 : 1;
	}
	return filevercmp(a->name, b->name);
}

#define SORT_NAME fi_dir_ver
#define SORT_TYPE struct file_info
#define SORT_CMP(x, y) (sorter(&x, &y))
#include "sort/sort.h"

struct opts_t opts;

static char usage[] = "Usage: lsc [option ...] [file ...]\n"
	"  -C when  Use colours (never, always or auto)\n"
	"  -F       Append file type indicator\n"
	"  -a       Show all files\n"
	"  -c       Use ctime\n"
	"  -S       Sort by size\n"
	"  -r       Reverse sort\n"
	"  -t       Sort by time\n"
	"  -h       Show this help";

void parse_args(int argc, char **argv) {
	opts.color = COLOR_AUTO;
	opts.sorter = fi_dir_ver_tim_sort;
	//opts.sorter = sort_by_ver;
	opts.rest = malloc(sizeof(char *) * (size_t)argc);
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
				opts.reverse = true;
				break;
			case 't':
				//opts.sorter = sortByTime;
				break;
			case 'S':
				//opts.sorter = sortBySize;
				break;
			case 'C': ; //WTF?
				char *use;
				if (s[1] == '\0') {
					if (i+1 < argc) {
						// No argument
						fputs("option '-C' needs an argument", stderr);
						die(usage);
					}
					// Argument is next one
					use = argv[i+1];
					i++;
				} else {
					// Argument is part of this one
					use = s+1;
				}

				if (strcmp("never", use) == 0) {
					opts.color = COLOR_NEVER;
				} else if (strcmp("always", use) == 0) {
					opts.color = COLOR_ALWAYS;
				} else if (strcmp("auto", use) == 0) {
					opts.color = COLOR_AUTO;
				} else {
					fprintf(stderr, "invalid argument to option '-C': \"%s\"\n", use);
					die(usage);
				}

				break;
			case 'h':
				die(usage);
			default:
				fprintf(stderr, "unsupported option '%c'\n", f);
				die(usage);
			}
		}
	}
	if (restc == 0) {
		opts.rest[0] = ".";
		restc = 1;
	}
	opts.restc = restc;
}
