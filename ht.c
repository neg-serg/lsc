#include <stddef.h>
#include <stdbool.h>
#include "slice.h"
#include "ht.h"
#define HT(x) ssht_ ## x
#include "genht/ht.c"
