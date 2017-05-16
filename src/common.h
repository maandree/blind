/* See LICENSE file for copyright and license details. */
#if defined(__clang__)
# pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
# pragma clang diagnostic ignored "-Wcomma"
# pragma clang diagnostic ignored "-Wcast-align"
# pragma clang diagnostic ignored "-Wassign-enum"
# pragma clang diagnostic ignored "-Wfloat-equal"
# pragma clang diagnostic ignored "-Wformat-nonliteral"
# pragma clang diagnostic ignored "-Wcovered-switch-default"
#elif defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include "stream.h"
#include "util.h"
#include "video-math.h"

#include <arpa/inet.h>
#if defined(HAVE_EPOLL)
# include <sys/epoll.h>
#endif
#include <sys/mman.h>
#include <sys/stat.h>
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
