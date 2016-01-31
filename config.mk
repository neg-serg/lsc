CPPFLAGS ?=
CFLAGS ?= -Wall -Wextra -pedantic -march=native -O2 -s -flto -pipe -fno-builtin-memcpy -fno-builtin-mempcpy 
CC ?= gcc
LDFLAGS ?= $(CFLAGS)
