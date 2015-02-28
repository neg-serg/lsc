#ifndef LSC_HT_H
#define LSC_HT_H

typedef struct te_key {
	char *key;
	size_t len;
} te_key_t;

typedef char *te_value_t;
#define HT(x) te_ ## x
#include "genht/ht.h"

#endif
