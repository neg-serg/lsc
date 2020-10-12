CFLAGS ?= -O2 -pipe -Wall -Wextra -pedantic -g \
  -fno-align-functions -fno-align-jumps -fno-align-labels -fno-align-loops 
CFLAGS += -std=c99
CPPFLAGS += -D_XOPEN_SOURCE=700
all: lsc
clean:; rm -f lsc
.PHONY: clean
