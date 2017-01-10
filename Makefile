CONFIGFILE = config.mk
include $(CONFIGFILE)

BIN =\
	vu-arithm\
	vu-colour-srgb\
	vu-concat\
	vu-cut\
	vu-dissolve\
	vu-flip\
	vu-flop\
	vu-from-image\
	vu-invert-luma\
	vu-next-frame\
	vu-read-head\
	vu-repeat\
	vu-reverse\
	vu-set-alpha\
	vu-set-luma\
	vu-set-saturation\
	vu-single-colour\
	vu-split\
	vu-stack\
	vu-to-image\
	vu-write-head

all: $(BIN)

%: %.o util.o stream.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: src/%.c src/*.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	-rm $(BIN) $(BIN:=.o) util.o stream.o

.PHONY: all clean
