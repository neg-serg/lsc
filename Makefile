-include config.mk

bin = lsc
src = filevercmp.c util.c lsc.c id.c ls_colors.c
obj = $(src:.c=.o)
dep = $(src:.c=.d)

CPPFLAGS += -MMD -MP -D_POSIX_C_SOURCE=1
CFLAGS += -std=c99

$(bin): $(obj)

clean:
	rm -f $(bin) $(obj) $(dep)

fuzz:
	afl-clang-fast -static -g $(CFLAGS) $(CPPFLAGS) -DTEST ls_colors.c util.c -o ls_colors
	afl-clang-fast -static -g $(CFLAGS) $(CPPFLAGS) -DTEST filevercmp.c util.c -o filevercmp

-include $(dep)

.PHONY: clean
