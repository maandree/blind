# blind version
VERSION = 1.1

# Paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# You may want to remove -DHAVE_PRCTL and -DHAVE_EPOLL from CPPFLAGS if you are not using Linux.
CFLAGS   = -std=c11 -Wall -pedantic -O2
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_FILE_OFFSET_BITS=64 -DHAVE_PRCTL -DHAVE_EPOLL
LDFLAGS  = -lm -s
