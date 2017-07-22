CONFIGFILE = config.mk
include $(CONFIGFILE)


BIN =\
	blind-affine-colour\
	blind-apply-kernel\
	blind-apply-palette\
	blind-arithm\
	blind-cat-cols\
	blind-cat-rows\
	blind-chroma-key\
	blind-colour-ciexyz\
	blind-colour-srgb\
	blind-compress\
	blind-concat\
	blind-cone-gradient\
	blind-convert\
	blind-coordinate-field\
	blind-crop\
	blind-cross-product\
	blind-cut\
	blind-decompress\
	blind-disperse\
	blind-dissolve\
	blind-dot-product\
	blind-double-sine-wave\
	blind-dual-key\
	blind-extend\
	blind-extract-alpha\
	blind-find-rectangle\
	blind-flip\
	blind-flop\
	blind-from-image\
	blind-from-named\
	blind-from-portable\
	blind-from-text\
	blind-from-video\
	blind-gauss-blur\
	blind-get-colours\
	blind-hexagon-tessellation\
	blind-interleave\
	blind-invert-luma\
	blind-invert-matrix\
	blind-linear-gradient\
	blind-make-kernel\
	blind-matrix-orthoproject\
	blind-matrix-reflect\
	blind-matrix-rotate\
	blind-matrix-scale\
	blind-matrix-shear\
	blind-matrix-translate\
	blind-matrix-transpose\
	blind-mosaic\
	blind-mosaic-corners\
	blind-mosaic-edges\
	blind-multiply-matrices\
	blind-next-frame\
	blind-norm\
	blind-quaternion-product\
	blind-premultiply\
	blind-radial-gradient\
	blind-read-head\
	blind-rectangle-tessellation\
	blind-repeat\
	blind-repeat-tessellation\
	blind-reverse\
	blind-rewrite-head\
	blind-round-wave\
	blind-sawtooth-wave\
	blind-set-alpha\
	blind-set-luma\
	blind-set-saturation\
	blind-single-colour\
	blind-sinc-wave\
	blind-sine-wave\
	blind-skip-pattern\
	blind-spectrum\
	blind-spiral-gradient\
	blind-split\
	blind-split-chans\
	blind-split-cols\
	blind-split-rows\
	blind-square-gradient\
	blind-stack\
	blind-tee\
	blind-time-blur\
	blind-triangular-wave\
	blind-to-image\
	blind-to-named\
	blind-to-portable\
	blind-to-text\
	blind-to-video\
	blind-translate\
	blind-transpose\
	blind-triangle-tessellation\
	blind-unpremultiply\
	blind-vector-projection\
	blind-write-head\
	blind-kernel\
	blind-temporal-mean

# TODO Not tested yet (and doesn't have any manpages):
#    blind-kernel
#    blind-temporal-mean

SCRIPTS =\
	blind-rotate-90\
	blind-rotate-180\
	blind-rotate-270

COMMON_OBJ =\
	util.o\
	stream.o

HDR =\
	arg.h\
	common.h\
	define-functions.h\
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
	util/fshut.h\
	video-math.h

MISCFILES =\
	Makefile\
	config.mk\
	LICENSE\
	README\
	TODO

EXAMPLEDIRS =\
	inplace-flop\
	reverse\
	split

EXAMPLEFILES =\
	inplace-flop/Makefile\
	reverse/Makefile\
	split/Makefile

COMMON_SRC = $(COMMON_SRC:.o=.c)
SRC = $(BIN:=.c) $(COMMON_SRC)
MAN1 = $(BIN:=.1) $(SCRIPTS:=.1)
MAN7 = blind.7


all: $(BIN)
mcb: blind-mcb

%: %.o $(COMMON_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: src/%.c src/*.h src/*/*.h platform.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.mcb.o: src/%.c src/*.h src/*/*.h platform.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Dmain="$$(printf 'main_%s\n' $* | tr -- - _)" -c -o $@ $<

blind-mcb.c: Makefile
	printf '#include <%s.h>\n' stdio string > blind-mcb.c
	printf 'int main_%s(int argc, char *argv[]);\n' $(BIN) | tr -- - _ >> blind-mcb.c
	printf 'int main(int argc, char *argv[]) {\n' >> blind-mcb.c
	printf 'char *cmd = strrchr(*argv, '"'/'"');\n' >> blind-mcb.c
	printf 'cmd = cmd ? cmd + 1 : *argv;\n' >> blind-mcb.c
	for c in $(BIN); do \
		printf 'if (!strcmp(cmd, "%s"))\n\treturn main_%s(argc, argv);\n' "$$c" "$$c" | \
			sed '/^\t/s/-/_/g'; \
	done >> blind-mcb.c
	printf 'fprintf(stderr, "Invalid command: %%s\\n", cmd);\n' >> blind-mcb.c
	printf 'return 1;\n' >> blind-mcb.c
	printf '}\n' >> blind-mcb.c

blind-mcb: blind-mcb.o $(BIN:=.mcb.o) $(COMMON_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

generate-macros: src/generate-macros.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LDFLAGS)

platform.h: generate-macros
	./generate-macros > platform.h

install: all
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin"
	cp -f -- $(BIN) $(SCRIPTS) "$(DESTDIR)$(PREFIX)/bin"
	cd -- "$(DESTDIR)$(PREFIX)/bin" && chmod 755 $(BIN) $(SCRIPTS)
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

install-mcb: mcb
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin"
	for c in $(BIN); do \
		$(LN) -f -- blind-single-colour "$(DESTDIR)$(PREFIX)/bin/$$c"; done
	rm -f -- "$(DESTDIR)$(PREFIX)/bin/blind-single-colour"
	cp -f -- blind-mcb "$(DESTDIR)$(PREFIX)/bin/blind-single-colour"
	cp -f -- $(SCRIPTS) "$(DESTDIR)$(PREFIX)/bin"
	cd -- "$(DESTDIR)$(PREFIX)/bin" && chmod 755 -- blind-single-colour $(SCRIPTS)
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
	cd -- "$(DESTDIR)$(PREFIX)/bin" && rm -f $(BIN) $(SCRIPTS)
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
	-rm -f $(BIN) *.o blind-$(VERSION).tar.gz platform.h generate-macros
	-rm -f blind-mcb.c blind-mcb.o blind-mcb
	-rm -rf "blind-$(VERSION)"


.PHONY: all mcb install install-mcb uninstall dist clean
.PRECIOUS: $(COMMON_OBJ) platform.h
