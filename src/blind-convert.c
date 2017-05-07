/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <string.h>

USAGE("pixel-format ...")

static void (*outconv)(double x, double y, double z, double a);

#define INCONV(TYPE)\
	do {\
		TYPE *pixel, x, y, z, a;\
		size_t n;\
		do {\
			pixel = (TYPE *)stream->buf;\
			for (n = stream->ptr / stream->pixel_size; n--; pixel += 4) {\
				x = (TYPE)(pixel[0]);\
				y = (TYPE)(pixel[1]);\
				z = (TYPE)(pixel[2]);\
				a = (TYPE)(pixel[3]);\
				outconv(x, y, z, a);\
			}\
			n = stream->ptr - (stream->ptr % stream->pixel_size);\
			memmove(stream->buf, stream->buf + n, stream->ptr -= n);\
		} while (eread_stream(stream, SIZE_MAX));\
		if (stream->ptr)\
			eprintf("%s: incomplete frame\n", stream->file);\
	} while (0)

#define OUTCONV(TYPE)\
	do {\
		TYPE pixel[4];\
		pixel[0] = (TYPE)x;\
		pixel[1] = (TYPE)y;\
		pixel[2] = (TYPE)z;\
		pixel[3] = (TYPE)a;\
		ewriteall(STDOUT_FILENO, pixel, sizeof(pixel), "<stdout>");\
	} while (0)

static void inconv_xyza (struct stream *stream) {INCONV(double);}
static void inconv_xyzaf(struct stream *stream) {INCONV(float);}

static void outconv_xyza (double x, double y, double z, double a) {OUTCONV(double);}
static void outconv_xyzaf(double x, double y, double z, double a) {OUTCONV(float);}

int
main(int argc, char *argv[])
{
	struct stream stream;
	const char *pixfmt;
	void (*inconv)(struct stream *stream);

	UNOFLAGS(!argc);

	eopen_stream(&stream, NULL);

	if (!strcmp(stream.pixfmt, "xyza"))
		inconv = inconv_xyza;
	else if (!strcmp(stream.pixfmt, "xyza f"))
		inconv = inconv_xyzaf;
	else
		eprintf("input pixel format %s is not supported\n", stream.pixfmt);

	pixfmt = stream.pixfmt;
	while (*argv)
		pixfmt = get_pixel_format(*argv++, pixfmt);

	if (!strcmp(pixfmt, "xyza"))
		outconv = outconv_xyza;
	else if (!strcmp(pixfmt, "xyza f"))
		outconv = outconv_xyzaf;
	else
		eprintf("output pixel format %s is not supported\n", pixfmt);

	strcpy(stream.pixfmt, pixfmt);
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	inconv(&stream);
	return 0;
}
