CFLAGS ?= -O2 -pipe -Wall -Wextra -pedantic -g
CFLAGS += -std=c99
CPPFLAGS += -D_XOPEN_SOURCE=700
all: lsc
clean:; rm -f lsc
.PHONY: clean
