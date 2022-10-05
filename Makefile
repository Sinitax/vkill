PREFIX ?= /usr/local
BINDIR ?= /bin

all: vkill

clean:
	rm -f vkill

vkill: vkill.c

install: vkill
	install -m755 vkill -T "$(DESTDIR)$(PREFIX)$(BINDIR)/vkill"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)$(BINDIR)/vkill"

.PHONY: all clean install uninstall
