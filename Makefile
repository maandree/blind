CONFIGFILE = config.mk
include $(CONFIGFILE)

BIN =\
	blind-arithm\
	blind-colour-ciexyz\
	blind-colour-srgb\
	blind-compress\
	blind-concat\
	blind-crop\
	blind-cut\
	blind-decompress\
	blind-dissolve\
	blind-extend\
	blind-flip\
	blind-flop\
	blind-from-image\
	blind-from-text\
	blind-from-video\
	blind-gauss-blur\
	blind-invert-luma\
	blind-next-frame\
	blind-read-head\
	blind-repeat\
	blind-reverse\
	blind-rewrite-head\
	blind-set-alpha\
	blind-set-luma\
	blind-set-saturation\
	blind-single-colour\
	blind-skip-pattern\
	blind-split\
	blind-stack\
	blind-time-blur\
	blind-to-image\
	blind-to-text\
	blind-to-video\
	blind-translate\
	blind-transpose\
	blind-write-head

SCRIPTS =\
	blind-rotate-90\
	blind-rotate-180\
	blind-rotate-270

MAN1 = $(BIN:=.1) $(SCRIPTS:=.1)
MAN7 = blind.7

SRC = $(BIN:=.c) util.c stream.c

HDR =\
	arg.h\
	stream.h\
	util.h\
	util/to.h\
	util/jobs.h\
	util/emalloc.h\
	util/eopen.h\
	util/endian.h\
	util/colour.h\
	util/io.h\
	util/efflush.h\
	util/efunc.h\
	util/eprintf.h\
	util/fshut.h

MISCFILES = Makefile config.mk LICENSE README TODO

EXAMPLEDIRS =\
	inplace-flop\
	reverse\
	split

EXAMPLEFILES =\
	inplace-flop/Makefile\
	reverse/Makefile\
	split/Makefile

all: $(BIN)

%: %.o util.o stream.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: src/%.c src/*.h src/*/*.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

install: all
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin"
	cp -f -- $(BIN) "$(DESTDIR)$(PREFIX)/bin"
	cd -- "$(DESTDIR)$(PREFIX)/bin" && chmod 755 $(BIN)
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man1"
	set -e && for m in $(MAN1); do \
		sed '1s/ blind$$/ " blind $(VERSION)"/g' \
		< "man/$$m" > "$(DESTDIR)$(MANPREFIX)/man1/$$m"; \
	done
	cd -- "$(DESTDIR)$(MANPREFIX)/man1" && chmod 644 $(MAN1)
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man7"
	set -e && for m in $(MAN7); do \
		sed '1s/ blind$$/ " blind $(VERSION)"/g' \
		< "man/$$m" > "$(DESTDIR)$(MANPREFIX)/man7/$$m"; \
	done
	cd -- "$(DESTDIR)$(MANPREFIX)/man7" && chmod 644 $(MAN7)

uninstall:
	cd -- "$(DESTDIR)$(PREFIX)/bin" && rm -f $(BIN)
	cd -- "$(DESTDIR)$(MANPREFIX)/man1" && rm -f $(MAN1)
	cd -- "$(DESTDIR)$(MANPREFIX)/man7" && rm -f $(MAN7)

dist:
	-rm -rf "blind-$(VERSION)"
	mkdir -p "blind-$(VERSION)/src/util" "blind-$(VERSION)/man"
	cp $(MISCFILES) $(SCRIPTS) "blind-$(VERSION)"
	cd man && cp $(MAN1) $(MAN7) "../blind-$(VERSION)/man"
	set -e && cd src && for s in $(SRC) $(HDR); do \
		cp "$$s" "../blind-$(VERSION)/src/$$s"; done
	set -e && for e in $(EXAMPLEDIRS); do \
		mkdir -p "blind-$(VERSION)/examples/$$e"; done
	set -e && cd examples && for e in $(EXAMPLEFILES); \
		do cp "$$e" "../blind-$(VERSION)/examples/$$e"; done
	tar -cf "blind-$(VERSION).tar" "blind-$(VERSION)"
	gzip -9 "blind-$(VERSION).tar"
	rm -rf "blind-$(VERSION)"

clean:
	-rm -f $(BIN) *.o blind-$(VERSION).tar.gz
	-rm -rf "blind-$(VERSION)"

.PHONY: all install uninstall dist clean
.PRECIOUS: util.o stream.o
