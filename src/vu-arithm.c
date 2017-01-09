/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

/* Because the syntax for a function returning a function pointer is disgusting. */
typedef void (*process_func)(struct stream *left, struct stream *right, size_t n);

static void
usage(void)
{
	eprintf("usage: %s operation right-hand-stream\n", argv0);
}

#define LIST_OPERATORS\
	X(add, *lh += rh)\
	X(sub, *lh -= rh)\
	X(mul, *lh *= rh)\
	X(div, *lh /= rh)\
	X(exp, *lh = pow(*lh, rh))\
	X(log, *lh = log(*lh) / log(rh))\
	X(min, *lh = *lh < rh ? *lh : rh)\
	X(max, *lh = *lh > rh ? *lh : rh)

#define X(NAME, ALGO)\
	static void\
	process_lf_##NAME(struct stream *left, struct stream *right, size_t n)\
	{\
		size_t i;\
		double *lh, rh;\
		for (i = 0; i < n; i += 4 * sizeof(double)) {\
			lh = ((double *)(left->buf + i)) + 0, rh = ((double *)(right->buf + i))[0];\
			ALGO;\
			lh = ((double *)(left->buf + i)) + 1, rh = ((double *)(right->buf + i))[1];\
			ALGO;\
			lh = ((double *)(left->buf + i)) + 2, rh = ((double *)(right->buf + i))[2];\
			ALGO;\
			lh = ((double *)(left->buf + i)) + 3, rh = ((double *)(right->buf + i))[3];\
			ALGO;\
		}\
	}
LIST_OPERATORS
#undef X

static process_func
get_lf_process(const char *operation)
{
#define X(NAME, ALGO)\
	if (!strcmp(operation, #NAME)) return process_lf_##NAME;
LIST_OPERATORS
#undef X
	eprintf("algorithm not recognised: %s\n", operation);
	return NULL;
}

int
main(int argc, char *argv[])
{
	struct stream left;
	struct stream right;
	ssize_t r;
	size_t i, n;
	process_func process = NULL;

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	if (argc != 2)
		usage();

	left.file = "<stdin>";
	left.fd = STDIN_FILENO;
	einit_stream(&left);

	right.file = argv[1];
	right.fd = open(right.file, O_RDONLY);
	if (right.fd < 0)
		eprintf("open %s:", right.file);
	einit_stream(&right);

	if (left.width != right.width || left.height != right.height)
		eprintf("videos do not have the same geometry\n");
	if (left.pixel_size != right.pixel_size)
		eprintf("videos use incompatible pixel formats\n");

	if (!strcmp(left.pixfmt, "xyza"))
		process = get_lf_process(argv[0]);
	else
		eprintf("pixel format %s is not supported, try xyza\n", left.pixfmt);

	for (;;) {
		if (left.ptr < sizeof(left.buf) && !eread_stream(&left, SIZE_MAX)) {
			close(left.fd);
			left.fd = -1;
			break;
		}
		if (right.ptr < sizeof(right.buf) && !eread_stream(&right, SIZE_MAX)) {
			close(right.fd);
			right.fd = -1;
			break;
		}

		n = left.ptr < right.ptr ? left.ptr : right.ptr;
		n -= n % left.pixel_size;
		left.ptr -= n;
		right.ptr -= n;

		process(&left, &right, n);

		for (i = 0; i < n; i += (size_t)r) {
			r = write(STDOUT_FILENO, left.buf + i, n - i);
			if (r < 0)
				eprintf("write <stdout>:");
		}

		if ((n & 3) || left.ptr != right.ptr) {
			memmove(left.buf,  left.buf  + n, left.ptr);
			memmove(right.buf, right.buf + n, right.ptr);
		}
	}

	if (right.fd >= 0)
		close(right.fd);

	for (i = 0; i < left.ptr; i += (size_t)r) {
		r = write(STDOUT_FILENO, left.buf + i, left.ptr - i);
		if (r < 0)
			eprintf("write <stdout>:");
	}

	if (left.fd >= 0) {
		for (;;) {
			left.ptr = 0;
			if (!eread_stream(&left, SIZE_MAX)) {
				close(left.fd);
				left.fd = -1;
				break;
			}

			for (i = 0; i < left.ptr; i += (size_t)r) {
				r = write(STDOUT_FILENO, left.buf + i, left.ptr - i);
				if (r < 0)
					eprintf("write <stdout>:");
			}
		}
	}

	return 0;
}
