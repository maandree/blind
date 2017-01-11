/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("[-w] saturation-stream")

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
	struct stream colour, satur;
	int whitepoint = 0;
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
	satur.fd = eopen(satur.file, O_RDONLY);
	einit_stream(&satur);

	if (!strcmp(colour.pixfmt, "xyza"))
		process = whitepoint ? process_xyza_w : process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	process_two_streams(&colour, &satur, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
