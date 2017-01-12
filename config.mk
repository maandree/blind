# You may want to remove -DHAVE_PRCTL from CPPFLAGS if you are not using Linux.

CFLAGS   = -std=c99 -Wall -pedantic -O2
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_FILE_OFFSET_BITS=64 -DHAVE_PRCTL
LDFLAGS  = -lm -s
