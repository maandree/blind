/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("[-F pixel-format] block-width block-height")

#define SET_XYZA(TYPE)\
	(pixwidth *= sizeof(double),\
	 colours = alloca(4 * pixwidth),\
	 ((TYPE *)colours)[ 0] = (TYPE)(0.0000 * D65_XYZ_X),\
	 ((TYPE *)colours)[ 1] = (TYPE)(0.0000),\
	 ((TYPE *)colours)[ 2] = (TYPE)(0.0000 * D65_XYZ_Z),\
	 ((TYPE *)colours)[ 3] = (TYPE)1,\
	 ((TYPE *)colours)[ 4] = (TYPE)(0.3333 * D65_XYZ_X),\
	 ((TYPE *)colours)[ 5] = (TYPE)(0.3333),\
	 ((TYPE *)colours)[ 6] = (TYPE)(0.3333 * D65_XYZ_Z),\
	 ((TYPE *)colours)[ 7] = (TYPE)1,\
	 ((TYPE *)colours)[ 8] = (TYPE)(0.6667 * D65_XYZ_X),\
	 ((TYPE *)colours)[ 9] = (TYPE)(0.6667),\
	 ((TYPE *)colours)[10] = (TYPE)(0.6667 * D65_XYZ_Z),\
	 ((TYPE *)colours)[11] = (TYPE)1,\
	 ((TYPE *)colours)[12] = (TYPE)(1.0000 * D65_XYZ_X),\
	 ((TYPE *)colours)[13] = (TYPE)(1.0000),\
	 ((TYPE *)colours)[14] = (TYPE)(1.0000 * D65_XYZ_Z),\
	 ((TYPE *)colours)[15] = (TYPE)1)

static struct stream stream = { .width = 0, .height = 0, .frames = 1 };

int
main(int argc, char *argv[])
{
	size_t width, height;
	const char *pixfmt = "xyza";
	size_t pixwidth = 4;
	char *colours;
	size_t x1, y1, x2, y2;

	ARGBEGIN {
	case 'F':
		pixfmt = UARGF();
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 2)
		usage();

	width  = etozu_arg("block-width", argv[0], 1, SIZE_MAX);
	height = etozu_arg("block-height", argv[1], 1, SIZE_MAX);

	pixfmt = get_pixel_format(pixfmt, "xyza");
	if (!strcmp(pixfmt, "xyza"))
		SET_XYZA(double);
	else if (!strcmp(pixfmt, "xyza f"))
		SET_XYZA(float);
	else
		eprintf("pixel format %s is not supported, try xyza\n", pixfmt);

	strcpy(stream.pixfmt, pixfmt);
	stream.width  = 2 * width;
	stream.height = 2 * height;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	for (y1 = 0; y1 < 2; y1++)
		for (y2 = 0; y2 < height; y2++)
			for (x1 = 0; x1 < 2; x1++)
				for (x2 = 0; x2 < width; x2++)
					ewriteall(STDOUT_FILENO, colours + (y1 * 2 + x1) * pixwidth, pixwidth, "<stdout>");

	return 0;
}
