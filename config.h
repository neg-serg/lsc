#ifndef LSC_CONF_H
#define LSC_CONF_H

// Column delimiter
#define COL " "

// Symlink -> symlink target
#define N_SYM_DELIM " → "

// Number part of size
#define N_SIZE ""

#define C_ESC "\033["    // Start escape sequence
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

static const buf n_sizes[7] = {
	buf_const("B"), // Byte
	buf_const("K"), // Kibibyte
	buf_const("M"), // Mebibyte
	buf_const("G"), // Gibibyte
	buf_const("T"), // Tebibyte
	buf_const("P"), // Pebibyte
	buf_const("E"), // Exbibyte
};

static const buf c_sizes[7] = {
	buf_const(C_ESC "38;5;15m" "B" C_END), // Byte
	buf_const(C_ESC "38;5;10m" "K" C_END), // Kibibyte
	buf_const(C_ESC "38;5;14m" "M" C_END), // Mebibyte
	buf_const(C_ESC "38;5;12m" "G" C_END), // Gibibyte
	buf_const(C_END "T"),                  // Tebibyte
	buf_const(C_END "P"),                  // Pebibyte
	buf_const(C_END "E"),                  // Exbibyte
};

static const buf n_modes[15] = {
	buf_const("-"), // i_none

	buf_const("r"), // i_read
	buf_const("w"), // i_write
	buf_const("x"), // i_exec

	buf_const("d"), // i_dir
	buf_const("c"), // i_char
	buf_const("b"), // i_block
	buf_const("p"), // i_fifo
	buf_const("l"), // i_link

	buf_const("s"), // i_sock
	buf_const("S"), // i_uid
	buf_const("s"), // i_uid_exec
	buf_const("t"), // i_sticky
	buf_const("T"), // i_sticky_o

	buf_const("?"), // i_unknown
};

static const buf c_modes[15] = {
	buf_const(C_ESC  "38;5;0m"   "-"),         // i_none

	buf_const(C_ESC  "38;5;2m"   "r"),         // i_read
	buf_const(C_ESC  "38;5;216m" "w"),         // i_write
	buf_const(C_ESC  "38;5;131m" "x"),         // i_exec

	buf_const(C_ESC  "38;5;10m"  "d"   C_END), // i_dir
	buf_const(C_ESC  "0m"        "c"),         // i_char
	buf_const(C_ESC  "0m"        "b"),         // i_block
	buf_const(C_ESC  "0m"        "p"),         // i_fifo
	buf_const(C_ESC  "38;5;220m" "l"   C_END), // i_link

	buf_const(C_ESC  "38;5;161m" "s"),         // i_sock
	buf_const(C_ESC  "38;5;220m" "S"),         // i_uid
	buf_const(C_ESC  "38;5;161m" "s"),         // i_uid_exec
	buf_const(C_ESC  "38;5;220m" "t"),         // i_sticky
	buf_const(C_ESC  "38;5;220m" "T"   C_END), // i_sticky_o

	buf_const("?"),                            // i_unknown
};

#define C_FILE     "0"       
#define C_DIR      "38;5;12" 
#define C_LINK     "38;5;8;3"
#define C_FIFO     "38;5;126"
#define C_SOCK     "38;5;197"
#define C_BLK      "38;5;68" 
#define C_CHR      "38;5;113"
#define C_ORPHAN   "38;5;237"
#define C_EXEC     "38;5;174"
#define C_DOOR     "38;5;127"
#define C_SETUID   "38;5;137"
#define C_SETGID   "38;5;100"
#define C_STICKY   "38;5;86" 
#define C_OW       "38;5;220"
#define C_STICKYOW "38;5;139"

#define CL_DIR  "/"
#define CL_LINK "@"
#define CL_FIFO C_ESC "38;5;1m" "|" C_END
#define CL_SOCK C_ESC "38;5;5m" "=" C_END
#define CL_EXEC "*"

#define COLOR_SYM_TARGET true

#endif
