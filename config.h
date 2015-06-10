#ifndef LSC_CONF_H
#define LSC_CONF_H

// Column delimiters
#define nCol      " "

// Symlink -> symlink target
#define nSymDelim " → "

// Number part of size
#define nSize     ""

#define cESC      "\033["   // Start escape sequence
#define cEnd      cESC "0m" // End escape sequence

// Column delimiters
#define cCol      " "

// Symlink -> symlink target
#define cSymDelim " " cESC "38;5;8m" "→" cEnd " "

// Symlink color
#define cSym "38;5;8;3"

// Colours of relative times
#define cSecond   cESC "38;5;12m"
#define cMinute   cESC "38;5;9m"
#define cHour     cESC "38;5;1m"
#define cDay      cESC "38;5;8m"
#define cWeek     cESC "38;5;237m"
#define cMonth    cESC "38;5;237m"
#define cYear     cESC "38;5;0m"

// Number part of size
#define cSize     cESC "38;5;216m"

static struct sstr nSizes[7] = {
	SSTR("B"), // Byte
	SSTR("K"), // Kibibyte
	SSTR("M"), // Mebibyte
	SSTR("G"), // Gibibyte
	SSTR("T"), // Tebibyte
	SSTR("P"), // Pebibyte
	SSTR("E"), // Exbibyte
};

static struct sstr cSizes[7] = {
	SSTR(cESC "38;5;15m" "B" cEnd), // Byte
	SSTR(cESC "38;5;10m" "K" cEnd), // Kibibyte
	SSTR(cESC "38;5;14m" "M" cEnd), // Mebibyte
	SSTR(cESC "38;5;12m" "G" cEnd), // Gibibyte
	SSTR(cEnd "T"),                 // Tebibyte
	SSTR(cEnd "P"),                 // Pebibyte
	SSTR(cEnd "E"),                 // Exbibyte
};

static struct sstr nModes[15] = {
	SSTR("—"), // i_none

	SSTR("r"), // i_read
	SSTR("w"), // i_write
	SSTR("x"), // i_exec

	SSTR("d"), // i_dir
	SSTR("c"), // i_char
	SSTR("b"), // i_block
	SSTR("p"), // i_fifo
	SSTR("l"), // i_link

	SSTR("s"), // i_sock
	SSTR("S"), // i_uid
	SSTR("s"), // i_uid_exec
	SSTR("t"), // i_sticky
	SSTR("T"), // i_sticky_o

	SSTR("?"), // i_unknown
};

static struct sstr cModes[15] = {
	SSTR(cESC  "38;5;0m"   "—"),        // i_none

	SSTR(cESC  "38;5;2m"   "r"),        // i_read
	SSTR(cESC  "38;5;216m" "w"),        // i_write
	SSTR(cESC  "38;5;131m" "x"),        // i_exec

	SSTR(cESC  "38;5;10m"  "d"   cEnd), // i_dir
	SSTR(cESC  "0m"        "c"),        // i_char
	SSTR(cESC  "0m"        "b"),        // i_block
	SSTR(cESC  "0m"        "p"),        // i_fifo
	SSTR(cESC  "38;5;220m" "l"   cEnd), // i_link

	SSTR(cESC  "38;5;161m" "s"),        // i_sock
	SSTR(cESC  "38;5;220m" "S"),        // i_uid
	SSTR(cESC  "38;5;161m" "s"),        // i_uid_exec
	SSTR(cESC  "38;5;220m" "t"),        // i_sticky
	SSTR(cESC  "38;5;220m" "T"   cEnd), // i_sticky_o

	SSTR("?"),                          // i_unknown
};

#endif
