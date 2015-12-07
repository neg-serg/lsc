src = fbuf.c filevercmp.c ht.c lsc.c slice.c util.c xxhash/xxhash.c
obj = $(src:.c=.o)

all: lsc

-include config.mk
-include dep.mk

syntax:
	@$(CC) $(src) $(CFLAGS) $(CPPFLAGS) -fsyntax-only

lsc: $(obj)
	# CC -o $@
	@$(CC) -o $@ $(obj) $(LDFLAGS)

%.o: %.c
	# CC -c $<
	@$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

dep.mk: $(src)
	# CC -M $^
	@$(CC) $(CFLAGS) -MM $^ > $@

clean:
	rm -f lsc $(obj)

.PHONY: all clean syntax
