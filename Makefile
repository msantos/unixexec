.PHONY: all clean test

PROG=   unixexec

RM ?= rm

CFLAGS ?= -D_FORTIFY_SOURCE=2 -O2 -fstack-protector-strong \
          -Wformat -Werror=format-security \
          -fno-strict-aliasing
LDFLAGS += -Wl,-z,relro,-z,now

UNIXEXEC_CFLAGS ?= -g -Wall -fwrapv

CFLAGS += $(UNIXEXEC_CFLAGS)
LDFLAGS += $(UNIXEXEC_LDFLAGS)

all: $(PROG)

$(PROG):
	$(CC) $(CFLAGS) -o $@ $@.c $(LDFLAGS)

clean:
	-@$(RM) $(PROG)

test: $(PROG)
	@PATH=.:$(PATH) bats test
