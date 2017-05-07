/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("[-iw] mask-stream")

#define PROCESS(TYPE, INV)\
	do {\
		size_t i;\
		TYPE w, y, yo;\
		for (i = 0; i < n; i += colour->pixel_size) {\
			w = INV ((TYPE *)(mask->buf + i))[1];\
			w *= ((TYPE *)(mask->buf + i))[3];\
			yo = ((TYPE *)(colour->buf + i))[1];\
			y = (1 - yo) * w + yo * (1 - w);\
			((TYPE *)(colour->buf + i))[0] += (y - yo) * (TYPE)D65_XYZ_X;\
			((TYPE *)(colour->buf + i))[1] = y;\
			((TYPE *)(colour->buf + i))[2] += (y - yo) * (TYPE)D65_XYZ_Z;\
			/*
			 * Explaination:
			 *   Y is the luma and ((X / Xn - Y / Yn), (Z / Zn - Y / Yn))
			 *   is the chroma (according to CIELAB), where (Xn, Yn, Zn)
			 *   is the white point.
			 */\
		}\
	} while (0)

#define PROCESS_W(TYPE, INV)\
	do {\
		size_t i;\
		TYPE w, y, yo, X, Z;\
		for (i = 0; i < n; i += colour->pixel_size) {\
			X = ((TYPE *)(mask->buf + i))[0];\
			Z = ((TYPE *)(mask->buf + i))[2];\
			w = INV ((TYPE *)(mask->buf + i))[1];\
			w *= ((TYPE *)(mask->buf + i))[3];\
			yo = ((TYPE *)(colour->buf + i))[1];\
			y = (1 - yo) * w + yo * (1 - w);\
			((TYPE *)(colour->buf + i))[0] += (y - yo) * X;\
			((TYPE *)(colour->buf + i))[1] = y;\
			((TYPE *)(colour->buf + i))[2] += (y - yo) * Z;\
		}\
	} while (0)

static void process_xyza    (struct stream *colour, struct stream *mask, size_t n) {PROCESS(double,);}
static void process_xyza_i  (struct stream *colour, struct stream *mask, size_t n) {PROCESS(double, 1 -);}
static void process_xyza_w  (struct stream *colour, struct stream *mask, size_t n) {PROCESS_W(double,);}
static void process_xyza_iw (struct stream *colour, struct stream *mask, size_t n) {PROCESS_W(double, 1 -);}
static void process_xyzaf   (struct stream *colour, struct stream *mask, size_t n) {PROCESS(float,);}
static void process_xyzaf_i (struct stream *colour, struct stream *mask, size_t n) {PROCESS(float, 1 -);}
static void process_xyzaf_w (struct stream *colour, struct stream *mask, size_t n) {PROCESS_W(float,);}
static void process_xyzaf_iw(struct stream *colour, struct stream *mask, size_t n) {PROCESS_W(float, 1 -);}

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
	else if (!strcmp(colour.pixfmt, "xyza f"))
		process = invert ? whitepoint ? process_xyzaf_iw : process_xyzaf_i
				 : whitepoint ? process_xyzaf_w  : process_xyzaf;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	fprint_stream_head(stdout, &colour);
	efflush(stdout, "<stdout>");
	process_two_streams(&colour, &mask, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
