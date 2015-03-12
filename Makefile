all: lsc

files = fbuf.c filevercmp.c ht.c lsc.c util.c xxhash/xxhash.c

-include config.mk
CFLAGS += $(CPPFLAGS) -std=c11 -D_FILE_OFFSET_BITS=64

-include Makefile.dep

lsc: Makefile.dep
	# CC *.c -o $@
	@$(CC) $(files) $(CFLAGS) -o $@ $(LDFLAGS)

Makefile.dep: $(files)
	# CC -M	*.c > $@
	@$(CC) $(CFLAGS) -MM $^ | sed -e 's|^[^: ]*.o:|lsc:|' > $@

.PHONY: all
