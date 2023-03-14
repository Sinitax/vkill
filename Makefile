PREFIX ?= /usr/local
BINDIR ?= /bin

CFLAGS = -Wunused-variable -Wunused-function

all: vkill

clean:
	rm -f vkill

install:
	install -m755 vkill -T "$(DESTDIR)$(PREFIX)$(BINDIR)/vkill"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)$(BINDIR)/vkill"

.PHONY: all clean install uninstall
