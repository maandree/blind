/* See LICENSE file for copyright and license details. */
#include "arg.h"

#define ELEMENTSOF(ARRAY) (sizeof(ARRAY) / sizeof(*(ARRAY)))
#define MIN(A, B)         ((A) < (B) ? (A) : (B))
#define MAX(A, B)         ((A) > (B) ? (A) : (B))
#define CLIP(A, B, C)     ((B) < (A) ? (A) : (B) > (C) ? (C) : (B))

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
#include "util/endian.h"
#include "util/efunc.h"
