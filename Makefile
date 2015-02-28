all: lsc

-include config.mk
-include Makefile.dep

files = main.c filevercmp.c stat.c size.c args.c util.c time.c type.c lscolors.c modes.c ht.c xxhash/xxhash.c colorh.c fbuf.c fp.c conf.c
CFLAGS := $(CFLAGS) $(CPPFLAGS)

CFLAGS += -std=c11

lsc: Makefile.dep
	$(CC) $(files) $(CFLAGS) -o $@ $(LDFLAGS)

Makefile.dep: $(files)
	# CC -M	*.c > $@
	@$(CC) $(CFLAGS) -MM $^ | sed -e 's|^[^: ]*.o:|lsc:|' > $@

genht:
	cd genht
	sv up

.PHONY: all genht
