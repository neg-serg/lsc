-include config.mk

bin = lsc
src = filevercmp.c util.c lsc.c id.c ls_colors.c
obj = $(src:.c=.o)
dep = $(src:.c=.d)

CFLAGS ?= -flto -O2 -Wall -Wextra -pedantic
LDFLAGS ?= -flto
CFLAGS += -std=c11
CPPFLAGS += -MMD -MP -D_DEFAULT_SOURCE -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64

all: $(bin)

$(bin): $(obj)

clean: ; rm -f $(bin) $(obj) $(dep)

-include $(dep)

.PHONY: all clean
