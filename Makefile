CONFIGFILE = config.mk
include $(CONFIGFILE)

BIN =\
	vu-arithm\
	vu-colour-srgb\
	vu-concat\
	vu-crop\
	vu-cut\
	vu-dissolve\
	vu-extend\
	vu-flip\
	vu-flop\
	vu-from-image\
	vu-gauss-blur\
	vu-invert-luma\
	vu-next-frame\
	vu-read-head\
	vu-repeat\
	vu-reverse\
	vu-rewrite-head\
	vu-set-alpha\
	vu-set-luma\
	vu-set-saturation\
	vu-single-colour\
	vu-split\
	vu-stack\
	vu-to-image\
	vu-to-text\
	vu-transpose\
	vu-write-head

all: $(BIN)

%: %.o util.o stream.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: src/%.c src/*.h src/*/*.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	-rm $(BIN) *.o

.PHONY: all clean
.PRECIOUS: util.o stream.o
