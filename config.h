#ifndef LSC_CONF_H
#define LSC_CONF_H

// Column delimiter
#define COL " "

// Symlink -> symlink target
#define N_SYM_DELIM " → "

// Number part of size
#define N_SIZE ""

#define C_ESC "\033["   // Start escape sequence
#define C_END C_ESC "0m" // End escape sequence

// Symlink -> symlink target
#define C_SYM_DELIM " " C_ESC "38;5;8m" "→" C_END " "

// Colours of relative times
#define C_SECOND C_ESC "38;5;12m"
#define C_MINUTE C_ESC "38;5;9m"
#define C_HOUR   C_ESC "38;5;1m"
#define C_DAY    C_ESC "38;5;8m"
#define C_WEEK   C_ESC "38;5;237m"
#define C_MONTH  C_ESC "38;5;237m"
#define C_YEAR   C_ESC "38;5;0m"

// Number part of size
#define C_SIZE     C_ESC "38;5;216m"

static const struct sstr n_sizes[7] = {
	SSTR("B"), // Byte
	SSTR("K"), // Kibibyte
	SSTR("M"), // Mebibyte
	SSTR("G"), // Gibibyte
	SSTR("T"), // Tebibyte
	SSTR("P"), // Pebibyte
	SSTR("E"), // Exbibyte
};

static const struct sstr c_sizes[7] = {
	SSTR(C_ESC "38;5;15m" "B" C_END), // Byte
	SSTR(C_ESC "38;5;10m" "K" C_END), // Kibibyte
	SSTR(C_ESC "38;5;14m" "M" C_END), // Mebibyte
	SSTR(C_ESC "38;5;12m" "G" C_END), // Gibibyte
	SSTR(C_END "T"),                 // Tebibyte
	SSTR(C_END "P"),                 // Pebibyte
	SSTR(C_END "E"),                 // Exbibyte
};

static const struct sstr n_modes[15] = {
	SSTR("-"), // i_none

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

static const struct sstr c_modes[15] = {
	SSTR(C_ESC  "38;5;0m"   "-"),        // i_none

	SSTR(C_ESC  "38;5;2m"   "r"),        // i_read
	SSTR(C_ESC  "38;5;216m" "w"),        // i_write
	SSTR(C_ESC  "38;5;131m" "x"),        // i_exec

	SSTR(C_ESC  "38;5;10m"  "d"   C_END), // i_dir
	SSTR(C_ESC  "0m"        "c"),        // i_char
	SSTR(C_ESC  "0m"        "b"),        // i_block
	SSTR(C_ESC  "0m"        "p"),        // i_fifo
	SSTR(C_ESC  "38;5;220m" "l"   C_END), // i_link

	SSTR(C_ESC  "38;5;161m" "s"),        // i_sock
	SSTR(C_ESC  "38;5;220m" "S"),        // i_uid
	SSTR(C_ESC  "38;5;161m" "s"),        // i_uid_exec
	SSTR(C_ESC  "38;5;220m" "t"),        // i_sticky
	SSTR(C_ESC  "38;5;220m" "T"   C_END), // i_sticky_o

	SSTR("?"),                          // i_unknown
};

#define C_FILE       "0"       
#define C_DIR        "38;5;12" 
#define C_LINK       "38;5;8;3"
#define C_FIFO       "38;5;126"
#define C_SOCK       "38;5;197"
#define C_BLK        "38;5;68" 
#define C_CHR        "38;5;113"
#define C_ORPHAN     "38;5;236"
#define C_EXEC       "38;5;174"
#define C_DOOR       "38;5;127"
#define C_SETUID     "38;5;137"
#define C_SETGID     "38;5;100"
#define C_STICKY     "38;5;86" 
#define C_OW         "38;5;220"
#define C_STICKYOW   "38;5;139"

#define COLOR_SYM_TARGET true

#endif
