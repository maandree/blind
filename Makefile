CONFIGFILE = config.mk
include $(CONFIGFILE)

BIN =\
	blind-arithm\
	blind-colour-srgb\
	blind-concat\
	blind-crop\
	blind-cut\
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
	blind-split\
	blind-stack\
	blind-time-blur\
	blind-to-image\
	blind-to-text\
	blind-to-video\
	blind-transpose\
	blind-write-head

SCRIPTS =\
	blind-rotate-90\
	blind-rotate-180\
	blind-rotate-270

MAN = $(BIN:=.1) $(SCRIPTS:=.1)

all: $(BIN)

%: %.o util.o stream.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: src/%.c src/*.h src/*/*.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	-rm $(BIN) *.o

.PHONY: all clean
.PRECIOUS: util.o stream.o
