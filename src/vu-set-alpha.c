/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s [-i] colour-stream alpha-stream\n", argv0);
}

static void
process_xyza(struct stream *colour, struct stream *alpha, size_t n)
{
	size_t i;
	double a;
	for (i = 0; i < n; i += colour->pixel_size) {
		a = ((double *)(alpha->buf + i))[1];
		a *= ((double *)(alpha->buf + i))[3];
		((double *)(colour->buf + i))[3] *= a;
	}
}

static void
process_xyza_i(struct stream *colour, struct stream *alpha, size_t n)
{
	size_t i;
	double a;
	for (i = 0; i < n; i += colour->pixel_size) {
		a = 1 - ((double *)(alpha->buf + i))[1];
		a *= ((double *)(alpha->buf + i))[3];
		((double *)(colour->buf + i))[3] *= a;
	}
}

int
main(int argc, char *argv[])
{
	int invert = 0;
	struct stream colour;
	struct stream alpha;
	ssize_t r;
	size_t i, n;
	void (*process)(struct stream *colour, struct stream *alpha, size_t n);

	ARGBEGIN {
	case 'i':
		invert = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 2)
		usage();

	colour.file = argv[0];
	colour.fd = open(colour.file, O_RDONLY);
	if (colour.fd < 0)
		eprintf("open %s:", colour.file);
	einit_stream(&colour);

	alpha.file = argv[1];
	alpha.fd = open(alpha.file, O_RDONLY);
	if (alpha.fd < 0)
		eprintf("open %s:", alpha.file);
	einit_stream(&alpha);

	if (colour.width != alpha.width || colour.height != alpha.height)
		eprintf("videos do not have the same geometry\n");
	if (colour.pixel_size != alpha.pixel_size)
		eprintf("videos use incompatible pixel formats\n");

	if (!strcmp(colour.pixfmt, "xyza"))
		process = invert ? process_xyza_i : process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	for (;;) {
		if (colour.ptr < sizeof(colour.buf) && !eread_stream(&colour, SIZE_MAX)) {
			close(colour.fd);
			colour.fd = -1;
			break;
		}
		if (alpha.ptr < sizeof(alpha.buf) && !eread_stream(&alpha, SIZE_MAX)) {
			close(colour.fd);
			alpha.fd = -1;
			break;
		}

		n = colour.ptr < alpha.ptr ? colour.ptr : alpha.ptr;
		n -= n % colour.pixel_size;
		colour.ptr -= n;
		alpha.ptr -= n;

		process(&colour, &alpha, n);

		for (i = 0; i < n; i += (size_t)r) {
			r = write(STDOUT_FILENO, colour.buf + i, n - i);
			if (r < 0)
				eprintf("write <stdout>:");
		}

		if ((n & 3) || colour.ptr != alpha.ptr) {
			memmove(colour.buf, colour.buf + n, colour.ptr);
			memmove(alpha.buf,  alpha.buf  + n, alpha.ptr);
		}
	}

	if (alpha.fd >= 0)
		close(alpha.fd);

	for (i = 0; i < colour.ptr; i += (size_t)r) {
		r = write(STDOUT_FILENO, colour.buf + i, colour.ptr - i);
		if (r < 0)
			eprintf("write <stdout>:");
	}

	if (colour.fd >= 0) {
		for (;;) {
			colour.ptr = 0;
			if (!eread_stream(&colour, SIZE_MAX)) {
				close(colour.fd);
				colour.fd = -1;
				break;
			}

			for (i = 0; i < colour.ptr; i += (size_t)r) {
				r = write(STDOUT_FILENO, colour.buf + i, colour.ptr - i);
				if (r < 0)
					eprintf("write <stdout>:");
			}
		}
	}

	return 0;
}
