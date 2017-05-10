/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <math.h>
#include <string.h>

USAGE("[-axyz] operation right-hand-stream")

static int skip_a = 0;
static int skip_x = 0;
static int skip_y = 0;
static int skip_z = 0;

/* Because the syntax for a function returning a function pointer is disgusting. */
typedef void (*process_func)(struct stream *left, struct stream *right, size_t n);

#define LIST_OPERATORS(PIXFMT, TYPE, SUFFIX)\
	X(add, *lh += rh,                                 PIXFMT, TYPE)\
	X(sub, *lh -= rh,                                 PIXFMT, TYPE)\
	X(mul, *lh *= rh,                                 PIXFMT, TYPE)\
	X(div, *lh /= rh,                                 PIXFMT, TYPE)\
	X(exp, *lh = pow##SUFFIX(*lh, rh),                PIXFMT, TYPE)\
	X(log, *lh = log##SUFFIX(*lh) / log##SUFFIX(rh),  PIXFMT, TYPE)\
	X(min, *lh = MIN(*lh, rh),                        PIXFMT, TYPE)\
	X(max, *lh = MAX(*lh, rh),                        PIXFMT, TYPE)\
	X(abs, *lh = fabs##SUFFIX(*lh - rh) + rh,         PIXFMT, TYPE)

#define C(CH, CHI, ALGO, TYPE)\
	(!skip_##CH ? ((lh = ((TYPE *)(left->buf + i)) + (CHI),\
			rh = ((TYPE *)(right->buf + i))[CHI],\
			(ALGO)), 0) : 0)

#define X(NAME, ALGO, PIXFMT, TYPE)\
	static void\
	process_##PIXFMT##_##NAME(struct stream *left, struct stream *right, size_t n)\
	{\
		size_t i;\
		TYPE *lh, rh;\
		for (i = 0; i < n; i += 4 * sizeof(TYPE)) {\
			C(x, 0, ALGO, TYPE);\
			C(y, 1, ALGO, TYPE);\
			C(z, 2, ALGO, TYPE);\
			C(a, 3, ALGO, TYPE);\
		}\
	}
LIST_OPERATORS(xyza, double,)
LIST_OPERATORS(xyzaf, float, f)
#undef X

static process_func
get_process_xyza(const char *operation)
{
#define X(NAME, ALGO, PIXFMT, TYPE)\
	if (!strcmp(operation, #NAME)) return process_##PIXFMT##_##NAME;
	LIST_OPERATORS(xyza, double,)
#undef X
	eprintf("algorithm not recognised: %s\n", operation);
	return NULL;
}

static process_func
get_process_xyzaf(const char *operation)
{
#define X(NAME, ALGO, PIXFMT, TYPE)\
	if (!strcmp(operation, #NAME)) return process_##PIXFMT##_##NAME;
	LIST_OPERATORS(xyzaf, float, f)
#undef X
	eprintf("algorithm not recognised: %s\n", operation);
	return NULL;
}

int
main(int argc, char *argv[])
{
	struct stream left, right;
	process_func process;

	ARGBEGIN {
	case 'a':
		skip_a = 1;
		break;
	case 'x':
		skip_x = 1;
		break;
	case 'y':
		skip_y = 1;
		break;
	case 'z':
		skip_z = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 2)
		usage();

	eopen_stream(&left, NULL);
	eopen_stream(&right, argv[1]);

	if (!strcmp(left.pixfmt, "xyza"))
		process = get_process_xyza(argv[0]);
	else if (!strcmp(left.pixfmt, "xyza f"))
		process = get_process_xyzaf(argv[0]);
	else
		eprintf("pixel format %s is not supported, try xyza\n", left.pixfmt);

	fprint_stream_head(stdout, &left);
	efflush(stdout, "<stdout>");
	process_two_streams(&left, &right, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
