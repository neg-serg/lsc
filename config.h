// Column delimiter
#define COL " "

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
#define C_SIZE C_ESC "38;5;216m"

static const char *c_sizes[7] = {
	C_ESC "38;5;15m" "B" C_END, // Byte
	C_ESC "38;5;10m" "K" C_END, // Kibibyte
	C_ESC "38;5;14m" "M" C_END, // Mebibyte
	C_ESC "38;5;12m" "G" C_END, // Gibibyte
	C_END "T",                  // Tebibyte
	C_END "P",                  // Pebibyte
	C_END "E",                  // Exbibyte
};

static const char *c_modes[15] = {
	C_ESC "38;5;0m"   "-",         // i_none

	C_ESC "38;5;2m"   "r",         // i_read
	C_ESC "38;5;216m" "w",         // i_write
	C_ESC "38;5;131m" "x",         // i_exec

	C_ESC "38;5;10m"  "d"   C_END, // i_dir
	C_ESC "0m"        "c",         // i_char
	C_ESC "0m"        "b",         // i_block
	C_ESC "0m"        "p",         // i_fifo
	C_ESC "38;5;220m" "l"   C_END, // i_link

	C_ESC "38;5;161m" "s",         // i_sock
	C_ESC "38;5;220m" "S",         // i_uid
	C_ESC "38;5;161m" "s",         // i_uid_exec
	C_ESC "38;5;220m" "t",         // i_sticky
	C_ESC "38;5;220m" "T"   C_END, // i_sticky_o

	"?",                           // i_unknown
};

static const char *c_kinds[14] = {
	"0",        // T_FILE
	"38;5;12",  // T_DIR
	"38;5;8;3", // T_LINK
	"38;5;126", // T_FIFO
	"38;5;197", // T_SOCK
	"38;5;68",  // T_BLK
	"38;5;113", // T_CHR
	"38;5;237", // T_ORPHAN
	"38;5;174", // T_EXEC
	"38;5;137", // T_SETUID
	"38;5;100", // T_SETGID
	"38;5;86",  // T_STICKY
	"38;5;220", // T_OW
	"38;5;139"  // T_STICKYOW
};

#define CL_DIR  "/"
#define CL_LINK "@"
#define CL_FIFO C_ESC "38;5;1m" "|" C_END
#define CL_SOCK C_ESC "38;5;5m" "=" C_END
#define CL_EXEC "*"
