-include config.mk

bin = lsc
src = filevercmp.c lsc.c util.c
obj = $(src:.c=.o)

prefix = /usr
bindir = $(prefix)/bin

CFLAGS ?= -O2 -Wall -Wextra -pedantic
CFLAGS += -std=c99
CPPFLAGS += -D_DEFAULT_SOURCE -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64

all: $(bin)

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

clean:
	rm -f $(bin) $(obj)

install: $(addprefix $(DESTDIR)$(bindir)/,$(bin))

$(DESTDIR)$(bindir)/%: %
	install -Dm755 $< $@

.PHONY: all clean install
