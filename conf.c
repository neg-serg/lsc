#include "conf.h"

const char *const nSizes[7] = {
	"B", // Byte
	"K", // Kibibyte
	"M", // Mebibyte
	"G", // Gibibyte
	"T", // Tebibyte
	"P", // Pebibyte
	"E", // Exbibyte
};

const char *const cSizes[] = {
	cESC "38;5;7;1m"  "B" cEnd, // Byte
	cESC "38;5;2;1m"  "K" cEnd, // Kibibyte
	cESC "38;5;14;1m" "M" cEnd, // Mebibyte
	cESC "38;5;12;1m" "G" cEnd, // Gibibyte
	cEnd "T",                   // Tebibyte
	cEnd "P",                   // Pebibyte
	cEnd "E",                   // Exbibyte
};
