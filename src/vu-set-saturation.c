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
	eprintf("usage: %s [-w] saturation-stream\n", argv0);
}

static void
process_xyza(struct stream *colour, struct stream *satur, size_t n)
{
	size_t i;
	double s, *x, *z, X, Z;
	X = D65_XYY_X / D65_XYY_Y;
	Z = 1 / D65_XYY_Y - 1 - X;
	for (i = 0; i < n; i += colour->pixel_size) {
		s = ((double *)(satur->buf + i))[1];
		s *= ((double *)(satur->buf + i))[3];
		x = ((double *)(colour->buf + i)) + 0;
		z = ((double *)(colour->buf + i)) + 2;
		*x = (*x - X) * s + X;
		*z = (*z - Z) * s + Z;
	}
}

static void
process_xyza_w(struct stream *colour, struct stream *satur, size_t n)
{
	size_t i;
	double s, *x, *z, X, Z;
	for (i = 0; i < n; i += colour->pixel_size) {
		X = ((double *)(satur->buf + i))[0];
		Z = ((double *)(satur->buf + i))[2];
		s = ((double *)(satur->buf + i))[1];
		s *= ((double *)(satur->buf + i))[3];
		x = ((double *)(colour->buf + i)) + 0;
		z = ((double *)(colour->buf + i)) + 2;
		*x = (*x - X) * s + X;
		*z = (*z - Z) * s + Z;
	}
}

int
main(int argc, char *argv[])
{
	struct stream colour;
	struct stream satur;
	int whitepoint = 0;
	ssize_t r;
	size_t i, n;
	void (*process)(struct stream *colour, struct stream *satur, size_t n) = NULL;

	ARGBEGIN {
	case 'w':
		whitepoint = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 1)
		usage();

	colour.file = "<stdin>";
	colour.fd = STDIN_FILENO;
	einit_stream(&colour);

	satur.file = argv[0];
	satur.fd = open(satur.file, O_RDONLY);
	if (satur.fd < 0)
		eprintf("open %s:", satur.file);
	einit_stream(&satur);

	if (colour.width != satur.width || colour.height != satur.height)
		eprintf("videos do not have the same geometry\n");
	if (colour.pixel_size != satur.pixel_size)
		eprintf("videos use incompatible pixel formats\n");

	if (!strcmp(colour.pixfmt, "xyza"))
		process = whitepoint ? process_xyza_w : process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	for (;;) {
		if (colour.ptr < sizeof(colour.buf) && !eread_stream(&colour, SIZE_MAX)) {
			close(colour.fd);
			colour.fd = -1;
			break;
		}
		if (satur.ptr < sizeof(satur.buf) && !eread_stream(&satur, SIZE_MAX)) {
			close(satur.fd);
			satur.fd = -1;
			break;
		}

		n = colour.ptr < satur.ptr ? colour.ptr : satur.ptr;
		n -= n % colour.pixel_size;
		colour.ptr -= n;
		satur.ptr -= n;

		process(&colour, &satur, n);

		for (i = 0; i < n; i += (size_t)r) {
			r = write(STDOUT_FILENO, colour.buf + i, n - i);
			if (r < 0)
				eprintf("write <stdout>:");
		}

		if ((n & 3) || colour.ptr != satur.ptr) {
			memmove(colour.buf, colour.buf + n, colour.ptr);
			memmove(satur.buf,  satur.buf  + n, satur.ptr);
		}
	}

	if (satur.fd >= 0)
		close(satur.fd);

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
