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
	double s, *x, y, *z;
	for (i = 0; i < n; i += colour->pixel_size) {
		s = ((double *)(satur->buf + i))[1];
		s *= ((double *)(satur->buf + i))[3];
		x = ((double *)(colour->buf + i)) + 0;
		y = ((double *)(colour->buf + i))[1];
		z = ((double *)(colour->buf + i)) + 2;
		*x = ((*x / D65_XYZ_X - y) * s + y) * D65_XYZ_X;
		*z = ((*z / D65_XYZ_Z - y) * s + y) * D65_XYZ_Z;
		/*
		 * Explaination:
		 *   Y is the luma and ((X / Xn - Y / Yn), (Z / Zn - Y / Yn))
		 *   is the chroma (according to CIELAB), where (Xn, Yn, Zn)
		 *   is the white point.
		 */
	}
}

static void
process_xyza_w(struct stream *colour, struct stream *satur, size_t n)
{
	size_t i;
	double s, *x, y, *z, X, Z;
	for (i = 0; i < n; i += colour->pixel_size) {
		X = ((double *)(satur->buf + i))[0];
		Z = ((double *)(satur->buf + i))[2];
		s = ((double *)(satur->buf + i))[1];
		s *= ((double *)(satur->buf + i))[3];
		x = ((double *)(colour->buf + i)) + 0;
		y = ((double *)(colour->buf + i))[1];
		z = ((double *)(colour->buf + i)) + 2;
		*x = ((*x / X - y) * s + y) * X;
		*z = ((*z / Z - y) * s + y) * Z;
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

	eopen_stream(&colour, NULL);
	eopen_stream(&satur, argv[0]);

	if (!strcmp(colour.pixfmt, "xyza"))
		process = whitepoint ? process_xyza_w : process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	fprint_stream_head(stdout, &colour);
	efflush(stdout, "<stdout>");
	process_two_streams(&colour, &satur, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
