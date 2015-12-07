CPPFLAGS = -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS = -Wall -Wextra -pedantic -march=native -O2 -s -flto -pipe
CC = gcc
LDFLAGS = $(CFLAGS)
