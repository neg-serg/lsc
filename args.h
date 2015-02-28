#ifndef LSC_ARGS_H
#define LSC_ARGS_H

//typedef int (*sort_func)(int a, int b);

enum use_color_t {
	COLOR_NEVER,
	COLOR_ALWAYS,
	COLOR_AUTO,
};

typedef void (sort_func)(struct file_info *, const size_t);

struct opts_t {
	char **rest;
	size_t  restc;
	sort_func *sorter;
	enum use_color_t color;
	bool all;
	bool classify;
	bool ctime;
	bool reverse;
};

extern struct opts_t opts;

void parse_args(int argc, char **argv);

#endif
