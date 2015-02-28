#ifndef LSC_CONF_H
#define LSC_CONF_H

#define MAX_COLOR_LEN 16
#define MAX_SIZE_LEN 32

// Column delimiters
#define nCol      " "

// Symlink -> symlink target
#define nSymDelim " → "

#define nNone     "—" // Nothing else applies

#define nRead     "r" // Readable
#define nWrite    "w" // Writeable
#define nExec     "x" // Executable

#define nDir      "d" // Directory
#define nChar     "c" // Character device
#define nBlock    "b" // Block device
#define nFifo     "p" // FIFO
#define nLink     "l" // Symlink

#define nSock     "s" // Socket
#define nUID      "S" // SUID
#define nUIDExec  "s" // SUID and executable
#define nSticky   "t" // Sticky
#define nStickyO  "T" // Sticky, writeable by others

// Relative times
#define nSecond   ""
#define nMinute   ""
#define nHour     ""
#define nDay      ""
#define nWeek     ""
#define nMonth    ""
#define nYear     ""

// Number part of size
#define nSize     ""

extern const char *const nSizes[7];

#define cESC      "\033["   // Start escape sequence
#define cEnd      cESC "0m" // End escape sequence

// Column delimiters
#define cCol      " "

// Symlink -> symlink target
#define cSymDelim " " cESC "38;5;8m" "→" cEnd " "

#define cNone     cESC "38;5;0m"     "—"      // Nothing else applies

#define cRead     cESC "38;5;2m"     "r"      // Readable
#define cWrite    cESC "38;5;216m"   "w"      // Writeable
#define cExec     cESC "38;5;131m"   "x" cEnd // Executable

#define cDir      cESC "38;5;2;1m"   "d" cEnd // Directory
#define cChar     cESC "0m"          "c"      // Character device
#define cBlock    cESC "0m"          "b"      // Block device
#define cFifo     cESC "0m"          "p"      // FIFO
#define cLink     cESC "38;5;220;1m" "l" cEnd // Symlink

#define cSock     cESC "38;5;161m"   "s"      // Socket
#define cUID      cESC "38;5;220m"   "S"      // SUID
#define cUIDExec  cESC "38;5;161m"   "s"      // SUID and executable
#define cSticky   cESC "38;5;220m"   "t"      // Sticky
#define cStickyO  cESC "38;5;220;1m" "T" cEnd// Sticky, writeable by others

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

extern const char *const cSizes[7];

#endif
