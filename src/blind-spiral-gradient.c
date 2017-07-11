/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("[-al] -w width -h height")

static int anticlockwise = 0;
static int logarithmic = 0;
static size_t width = 0;
static size_t height = 0;
static int with_params;
static int with_vector;


#define PROCESS(TYPE, SUFFIX)\
	static void\
	process_##SUFFIX(struct stream *stream)\
	{\
		typedef TYPE pixel_t[4];\
		pixel_t buf[BUFSIZ / sizeof(pixel_t)];\
		TYPE *params, x1, y1, x2, y2, b, r, u, v;\
		TYPE x, y, a = 0, e = 1, p = 1, k = 1, ep = 1;\
		TYPE x3 = 1, y3 = 0, rd = 1, P, R, Rx, Ry, Pe = 2, Re = 2, PRe = 0.5;\
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
			if (with_vector) {\
				x3 = (params)[8];\
				y3 = (params)[9];\
				Pe = (params)[12];\
				Re = (params)[13];\
				rd = (params)[14];\
				PRe = 1 / sqrt(Pe * Re);\
				b = sqrt(x3 * x3 + y3 * y3);\
				x3 /= b;\
				y3 /= b;\
			}\
			if (with_params) {\
				a = (params)[with_vector ? 16 : 8];\
				e = (params)[with_vector ? 17 : 9];\
				p = (params)[with_vector ? 18 : 10];\
				k = (params)[with_vector ? 19 : 11];\
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
					if (!x && !y) {\
						v = 0;\
					} else {\
						v = atan2(y, x);\
						if (anticlockwise)\
							v = -v;\
						v -= u;\
						v += 4 * (TYPE)M_PI;\
						v = mod(v, 2 * (TYPE)M_PI);\
					}\
					if (!with_vector) {\
						r = sqrt(x * x + y * y);\
					} else {\
						P = x * x3 + y * y3;\
						Rx = x - P * x3;\
						Ry = y - P * y3;\
						R = sqrt(Rx * Rx + Ry * Ry) / rd;\
						P = pow(abs(P), Pe);\
						R = pow(abs(R), Re);\
						r = pow(P + R, PRe);\
					}\
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

	if (stream.width > 5 || stream.height > 5 ||
	    stream.width * stream.height < 2 ||
	    stream.width * stream.height > 5)
		eprintf("<stdin>: each frame must contain exactly 2, 3, 4, or 5 pixels\n");

	with_params = (stream.width * stream.height) & 1;
	with_vector = stream.width * stream.height > 3;

	stream.width = width;
	stream.height = height;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	process(&stream);
	return 0;
}
