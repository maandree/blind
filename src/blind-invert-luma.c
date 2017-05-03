/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("[-iw] mask-stream")

static void
process_xyza(struct stream *colour, struct stream *mask, size_t n)
{
	size_t i;
	double w, y, yo;
	for (i = 0; i < n; i += colour->pixel_size) {
		w = ((double *)(mask->buf + i))[1];
		w *= ((double *)(mask->buf + i))[3];
		yo = ((double *)(colour->buf + i))[1];
		y = (1 - yo) * w + yo * (1 - w);
		((double *)(colour->buf + i))[0] += (y - yo) * D65_XYZ_X;
		((double *)(colour->buf + i))[1] = y;
		((double *)(colour->buf + i))[2] += (y - yo) * D65_XYZ_Z;
		/*
		 * Explaination:
		 *   Y is the luma and ((X / Xn - Y / Yn), (Z / Zn - Y / Yn))
		 *   is the chroma (according to CIELAB), where (Xn, Yn, Zn)
		 *   is the white point.
		 */
	}
}

static void
process_xyza_i(struct stream *colour, struct stream *mask, size_t n)
{
	size_t i;
	double w, y, yo;
	for (i = 0; i < n; i += colour->pixel_size) {
		w = 1 - ((double *)(mask->buf + i))[1];
		w *= ((double *)(mask->buf + i))[3];
		yo = ((double *)(colour->buf + i))[1];
		y = (1 - yo) * w + yo * (1 - w);
		((double *)(colour->buf + i))[0] += (y - yo) * D65_XYZ_X;
		((double *)(colour->buf + i))[1] = y;
		((double *)(colour->buf + i))[2] += (y - yo) * D65_XYZ_Z;
	}
}

static void
process_xyza_w(struct stream *colour, struct stream *mask, size_t n)
{
	size_t i;
	double w, y, yo, X, Z;
	for (i = 0; i < n; i += colour->pixel_size) {
		X = ((double *)(mask->buf + i))[0];
		Z = ((double *)(mask->buf + i))[2];
		w = ((double *)(mask->buf + i))[1];
		w *= ((double *)(mask->buf + i))[3];
		yo = ((double *)(colour->buf + i))[1];
		y = (1 - yo) * w + yo * (1 - w);
		((double *)(colour->buf + i))[0] += (y - yo) * X;
		((double *)(colour->buf + i))[1] = y;
		((double *)(colour->buf + i))[2] += (y - yo) * Z;
	}
}

static void
process_xyza_iw(struct stream *colour, struct stream *mask, size_t n)
{
	size_t i;
	double w, y, yo, X, Z;
	for (i = 0; i < n; i += colour->pixel_size) {
		X = ((double *)(mask->buf + i))[0];
		Z = ((double *)(mask->buf + i))[2];
		w = 1 - ((double *)(mask->buf + i))[1];
		w *= ((double *)(mask->buf + i))[3];
		yo = ((double *)(colour->buf + i))[1];
		y = (1 - yo) * w + yo * (1 - w);
		((double *)(colour->buf + i))[0] += (y - yo) * X;
		((double *)(colour->buf + i))[1] = y;
		((double *)(colour->buf + i))[2] += (y - yo) * Z;
	}
}

int
main(int argc, char *argv[])
{
	int invert = 0, whitepoint = 0;
	struct stream colour, mask;
	void (*process)(struct stream *colour, struct stream *mask, size_t n) = NULL;

	ARGBEGIN {
	case 'i':
		invert = 1;
		break;
	case 'w':
		whitepoint = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 1)
		usage();

	eopen_stream(&colour, NULL);
	eopen_stream(&mask, argv[0]);

	if (!strcmp(colour.pixfmt, "xyza"))
		process = invert ? whitepoint ? process_xyza_iw : process_xyza_i
				 : whitepoint ? process_xyza_w  : process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	fprint_stream_head(stdout, &colour);
	efflush(stdout, "<stdout>");
	process_two_streams(&colour, &mask, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
