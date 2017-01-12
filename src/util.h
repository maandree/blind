/* See LICENSE file for copyright and license details. */
#include "arg.h"

#define ELEMENTSOF(ARRAY) (sizeof(ARRAY) / sizeof(*(ARRAY)))

#define USAGE(SYNOPSIS)\
	static void usage(void)\
	{ eprintf("usage: %s%s%s\n", argv0, SYNOPSIS ? " " : "", SYNOPSIS); }

#include "util/eprintf.h"
#include "util/efflush.h"
#include "util/fshut.h"
#include "util/eopen.h"
#include "util/emalloc.h"
#include "util/to.h"
#include "util/colour.h"
#include "util/io.h"
#include "util/jobs.h"
