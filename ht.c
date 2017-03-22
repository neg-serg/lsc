#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "slice.h"
#include "ht.h"
#define HT(x) ssht_ ## x
#include "genht/ht.c"
