#define C_ESC "\033["    // Start escape sequence
#define C_END C_ESC "0m" // End escape sequence

// Symlink -> symlink target
#define C_SYM_DELIM " " C_ESC "38;5;56m" "→" C_END " "
#define C_SYM_DELIM_COLOR C_ESC "90m"

// User/group info
#define C_USERINFO C_ESC "90m"

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

// File mode string
#define C_NONE     C_ESC "90m" " "

#define C_READ     C_ESC "32m" "r"
#define C_WRITE    C_ESC "38;5;7m" "w"
#define C_EXEC     C_ESC "38;5;1m" "x"

#define C_FILE     C_ESC "90m" "."
#define C_DIR      C_ESC "38;5;4m" "d"
#define C_CHAR     C_ESC "38;5;24;1m" "c"
#define C_BLOCK    C_ESC "38;5;24m" "b"
#define C_FIFO     C_ESC "38;5;126m" "p"
#define C_LINK     C_ESC "38;5;05m" "l"

#define C_SOCK     C_ESC "38;5;161m" "s"
#define C_UID      C_ESC "38;5;220m" "S"
#define C_UID_EXEC C_ESC "38;5;161m" "s"
#define C_STICKY   C_ESC "38;5;220m" "t"
#define C_STICKY_O C_ESC "38;5;220m" "T"
#define C_UNKNOWN  C_END "?"

// Indicators
#define CL_DIR  C_END "/"
#define CL_LINK C_END "@"
#define CL_FIFO C_ESC "31m" "|"
#define CL_SOCK C_ESC "35m" "="
#define CL_EXEC C_END "*"
