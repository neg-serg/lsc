-include config.mk

bin = lsc
src = fbuf.c filevercmp.c ht.c lsc.c slice.c util.c xxhash/xxhash.c
obj = $(src:.c=.o)

prefix = /usr
bindir = $(prefix)/bin

all: $(bin)

-include dep.mk

CFLAGS += -std=c99
CPPFLAGS += -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64

$(bin): $(obj)
	# CC -o $@
	@$(CC) -o $@ $(obj) $(LDFLAGS)

%.o: %.c
	# CC -c $<
	@$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

dep.mk: $(src)
	# CC -M
	@$(CC) $(CFLAGS) -MM $^ > $@

clean:
	rm -f $(bin) $(obj)

install: $(addprefix $(DESTDIR)$(bindir)/,$(bin))

$(DESTDIR)$(bindir)/%: %
	# INSTALL $<
	@install -Dm755 $< $@

syntax:
	@$(CC) $(src) $(CFLAGS) $(CPPFLAGS) -fsyntax-only

.PHONY: all clean install syntax
