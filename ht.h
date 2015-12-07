#ifndef LSC_HT_H
#define LSC_HT_H

typedef buf ssht_key_t;

typedef char *ssht_value_t;
#define HT(x) ssht_ ## x
#include "genht/ht.h"

#endif
