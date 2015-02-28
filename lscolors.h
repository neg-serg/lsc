#ifndef LSC_LSCOLORS_H
#define LSC_LSCOLORS_H

extern bool color_sym_target;

void parse_ls_color(void);

const char *color(char *name, size_t len, enum indicator in);

#endif
