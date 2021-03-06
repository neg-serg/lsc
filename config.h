// Column delimiter
#define COL " "

#define C_ESC "\033["    // Start escape sequence
#define C_END C_ESC "0m" // End escape sequence

// Symlink -> symlink target
#define C_SYM_DELIM " " C_ESC "38;5;56m" "→" C_END " "

// Colours of relative times
#define C_SECOND C_ESC "38;5;4m"
#define C_MINUTE C_ESC "38;5;4m"
#define C_HOUR   C_ESC "38;5;4m"
#define C_DAY    C_ESC "38;5;4m"
#define C_WEEK   C_ESC "38;5;4m"
#define C_MONTH  C_ESC "38;5;4m"
#define C_YEAR   C_ESC "38;5;235m"

// Number part of size
#define C_SIZE C_ESC "38;5;7m"

static const char *c_sizes[7] = {
	C_ESC "01;38;5;7m"  "B" C_END,  // Byte
	C_ESC "01;38;5;2m"  "K" C_END, // Kibibyte
	C_ESC "01;38;5;14m" "M" C_END, // Mebibyte
	C_ESC "01;38;5;12m" "G" C_END, // Gibibyte
	C_END "T",                  // Tebibyte
	C_END "P",                  // Pebibyte
	C_END "E",                  // Exbibyte
};

static const char *c_modes[15] = {
	C_ESC "30m"        "-",         // i_none

	C_ESC "32m"        "r",         // i_read
	C_ESC "38;5;7m"    "w",         // i_write
	C_ESC "38;5;1m"    "x",         // i_exec

	C_ESC "01;38;5;2m" "d"   C_END, // i_dir
	C_ESC "0m"         "c",         // i_char
	C_ESC "0m"         "b",         // i_block
	C_ESC "0m"         "p",         // i_fifo
	C_ESC "38;5;233m"  "l"   C_END, // i_link

	C_ESC "38;5;161m"  "s",         // i_sock
	C_ESC "38;5;233m"  "S",         // i_uid
	C_ESC "38;5;161m"  "s",         // i_uid_exec
	C_ESC "38;5;233m"  "t",         // i_sticky
	C_ESC "38;5;233m"  "T"   C_END, // i_sticky_o

	"?",                            // i_unknown
};

static const char *c_kinds[14] = {
	"0",                      // T_FILE
	"38;5;4",                 // T_DIR
	"38;5;05",                // T_LINK
	"38;5;126",               // T_FIFO
	"01;38;5;075",            // T_SOCK
	"38;5;24",                // T_BLK
	"38;5;24;1",              // T_CHR
	"40;31;01",               // T_ORPHAN
	"00;04;32",               // T_EXEC
	"38;5;37",                // T_SETUID
	"38;5;37",                // T_SETGID
	"38;5;86",                // T_STICKY
	"38;5;86;48;5;234;1",     // T_OW
	"38;5;86;48;5;234"        // T_STICKYOW
};

#define CL_DIR  "/"
#define CL_LINK "@"
#define CL_FIFO C_ESC "31m" "|" C_END
#define CL_SOCK C_ESC "35m" "=" C_END
#define CL_EXEC "*"
