/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("[-al] -w width -h height")

static int anticlockwise = 0;
static int logarithmic = 0;
static size_t width = 0;
static size_t height = 0;
static int with_params;


#define PROCESS(TYPE, SUFFIX)\
	static void\
	process_##SUFFIX(struct stream *stream)\
	{\
		typedef TYPE pixel_t[4];\
		pixel_t buf[BUFSIZ / sizeof(pixel_t)];\
		TYPE *params, x1, y1, x2, y2, b, r, u, v;\
		TYPE x, y, a = 0, e = 1, p = 1, k = 1, ep = 1;\
		size_t ix, iy, ptr = 0;\
		for (;;) {\
			while (stream->ptr < stream->frame_size) {\
				if (!eread_stream(stream, stream->frame_size - stream->ptr)) {\
					ewriteall(STDOUT_FILENO, buf, ptr * sizeof(*buf), "<stdout>");\
					return;\
				}\
			}\
			params = (TYPE *)stream->buf;\
			x1 = (params)[0];\
			y1 = (params)[1];\
			x2 = (params)[4];\
			y2 = (params)[5];\
			if (with_params) {\
				a = (params)[8];\
				e = (params)[9];\
				p = (params)[10];\
				k = (params)[11];\
				ep = 1 / (e * p);\
			}\
			memmove(stream->buf, stream->buf + stream->frame_size,\
				stream->ptr -= stream->frame_size);\
			\
			x2 -= x1;\
			y2 -= y1;\
			u = atan2(y2, x2);\
			b = sqrt(x2 * x2 + y2 * y2);\
			if (logarithmic)\
				b = log(b);\
			b /= pow(2 * (TYPE)M_PI, e);\
			\
			for (iy = 0; iy < height; iy++) {\
				y = (TYPE)iy - y1;\
				for (ix = 0; ix < width; ix++) {\
					x = (TYPE)ix - x1;\
					v = atan2(y, x);\
					if (anticlockwise)\
						v = 1 - v;\
					v -= u;\
					v += 4 * (TYPE)M_PI;\
					v = mod(v, 2 * (TYPE)M_PI);\
					r = sqrt(x * x + y * y);\
					r -= a;\
					if (!logarithmic) {\
						r = pow(r / b, ep);\
						r = (r - v) / (2 * (TYPE)M_PI);\
					} else if (r) {\
						r = log(r / k);\
						r = pow(r / b, ep);\
						r = (r - v) / (2 * (TYPE)M_PI);\
					}\
					buf[ptr][0] = buf[ptr][1] = buf[ptr][2] = buf[ptr][3] = r;\
					if (++ptr == ELEMENTSOF(buf)) {\
						ewriteall(STDOUT_FILENO, buf, sizeof(buf), "<stdout>");\
						ptr = 0;\
					}\
				}\
			}\
		}\
	}

PROCESS(double, lf)
PROCESS(float, f)


int
main(int argc, char *argv[])
{
	struct stream stream;
	void (*process)(struct stream *stream);

	ARGBEGIN {
	case 'a':
		anticlockwise = 1;
		break;
	case 'l':
		logarithmic = 1;
		break;
	case 'w':
		width = etozu_flag('w', UARGF(), 1, SIZE_MAX);
		break;
	case 'h':
		height = etozu_flag('h', UARGF(), 1, SIZE_MAX);
		break;
	default:
		usage();
	} ARGEND;

	if (!width || !height || argc)
		usage();

	eopen_stream(&stream, NULL);

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_lf;
	else if (!strcmp(stream.pixfmt, "xyza f"))
		process = process_f;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	if (stream.width > 3 || stream.height > 3 ||
	    stream.width * stream.height < 2 ||
	    stream.width * stream.height > 3)
		eprintf("<stdin>: each frame must contain exactly 2 or 3 pixels\n");

	with_params = stream.width * stream.height == 3;

	stream.width = width;
	stream.height = height;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	process(&stream);
	return 0;
}
