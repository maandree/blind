/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("pixel-format ...")

static void (*outconv)(double *xyzas, size_t n);

#define INCONV(TYPE)\
	do {\
		double buf[sizeof(stream->buf) / (4 * sizeof(TYPE)) * 4];\
		double *interm;\
		TYPE *in;\
		size_t n, m;\
		do {\
			in = (TYPE *)stream->buf;\
			interm = buf;\
			n = stream->ptr / stream->pixel_size;\
			for (m = n; m--; in += 4, interm += 4) { \
				interm[0] = (double)(in[0]);\
				interm[1] = (double)(in[1]);\
				interm[2] = (double)(in[2]);\
				interm[3] = (double)(in[3]);\
			}\
			outconv(buf, n);\
			n *= stream->pixel_size;\
			memmove(stream->buf, stream->buf + n, stream->ptr -= n);\
		} while (eread_stream(stream, SIZE_MAX));\
		if (stream->ptr)\
			eprintf("%s: incomplete frame\n", stream->file);\
	} while (0)

#define OUTCONV(TYPE)\
	do {\
		TYPE *out = alloca(n * 4 * sizeof(TYPE));\
		size_t i, m = n * 4;\
		for (i = 0; i < m; i++)\
			out[i] = (TYPE)(xyzas[i]);\
		ewriteall(STDOUT_FILENO, out, n * 4 * sizeof(TYPE), "<stdout>");\
	} while (0)

static void inconv_xyza (struct stream *stream) {INCONV(double);}
static void inconv_xyzaf(struct stream *stream) {INCONV(float);}

static void outconv_xyza (double *xyzas, size_t n) {OUTCONV(double);}
static void outconv_xyzaf(double *xyzas, size_t n) {OUTCONV(float);}

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
