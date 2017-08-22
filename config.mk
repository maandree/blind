# blind version
VERSION = 1.1

# Paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# Dependencies
KORN_SHELL = bash
# Known to work: bash ksh
# Does not work: mksh loksh oksh dash

# Commands
LN = ln -s

# You may want to remove -DHAVE_PRCTL, -DHAVE_EPOLL, -DHAVE_TEE,
# and -DHAVE_SENDFILE from CPPFLAGS if you are not using Linux.
CFLAGS   = -std=c11 -Wall -pedantic -O2
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_GNU_SOURCE \
	   -D_FILE_OFFSET_BITS=64 -DHAVE_PRCTL -DHAVE_EPOLL -DHAVE_TEE -DHAVE_SENDFILE
LDFLAGS  = -lm -s
