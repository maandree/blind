/* See LICENSE file for copyright and license details. */
#if defined(__clang__)
# pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
# pragma clang diagnostic ignored "-Wcomma"
# pragma clang diagnostic ignored "-Wcast-align"
# pragma clang diagnostic ignored "-Wassign-enum"
# pragma clang diagnostic ignored "-Wfloat-equal"
# pragma clang diagnostic ignored "-Wformat-nonliteral"
# pragma clang diagnostic ignored "-Wcovered-switch-default"
# pragma clang diagnostic ignored "-Wfloat-conversion"
# pragma clang diagnostic ignored "-Wabsolute-value"
# pragma clang diagnostic ignored "-Wconditional-uninitialized"
# pragma clang diagnostic ignored "-Wunreachable-code-return"
#elif defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wfloat-equal"
# pragma GCC diagnostic ignored "-Wunsafe-loop-optimizations"
# pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include "../platform.h"
#include "stream.h"
#include "util.h"
#include "video-math.h"

#include <arpa/inet.h>
#if defined(HAVE_EPOLL)
# include <sys/epoll.h>
#endif
#include <sys/mman.h>
#if defined(HAVE_SENDFILE)
# include <sys/sendfile.h>
#endif
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <alloca.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifndef CMSG_ALIGN
# ifdef __sun__
#  define CMSG_ALIGN _CMSG_DATA_ALIGN
# else
#  define CMSG_ALIGN(len) (((len) + sizeof(long) - 1) & ~(sizeof(long) - 1))
# endif
#endif

#ifndef CMSG_SPACE
# define CMSG_SPACE(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + CMSG_ALIGN(len))
#endif

#ifndef CMSG_LEN
# define CMSG_LEN(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#endif
