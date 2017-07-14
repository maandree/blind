/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("[-es]")

static int equal = 0;
static int spiral = 0;


#define PROCESS(TYPE, SUFFIX)\
	static void\
	process_##SUFFIX(struct stream *stream)\
	{\
		size_t i, n;\
		TYPE x, y, z, a;\
		do {\
			n = stream->ptr / stream->pixel_size;\
			if (equal) {\
				for (i = 0; i < n; i++) {\
					a = ((TYPE *)(stream->buf))[4 * i + 3];\
					a = posmod(a, (TYPE)2);\
					a = a > 1 ? 2 - a : a;\
					if (spiral)\
						a = (a > (TYPE)0.5 ? 1 - a : a) * 2; \
					((TYPE *)(stream->buf))[4 * i + 0] = a;\
					((TYPE *)(stream->buf))[4 * i + 1] = a;\
					((TYPE *)(stream->buf))[4 * i + 2] = a;\
					((TYPE *)(stream->buf))[4 * i + 3] = a;\
				}\
			} else {\
				for (i = 0; i < n; i++) {\
					x = ((TYPE *)(stream->buf))[4 * i + 0];\
					y = ((TYPE *)(stream->buf))[4 * i + 1];\
					z = ((TYPE *)(stream->buf))[4 * i + 2];\
					a = ((TYPE *)(stream->buf))[4 * i + 3];\
					x = posmod(x, (TYPE)2);\
					y = posmod(y, (TYPE)2);\
					z = posmod(z, (TYPE)2);\
					a = posmod(a, (TYPE)2);\
					x = x > 1 ? 2 - x : x;\
					y = y > 1 ? 2 - y : y;\
					z = z > 1 ? 2 - z : z;\
					a = a > 1 ? 2 - a : a;\
					if (spiral) {\
						x = (x > (TYPE)0.5 ? 1 - x : x) * 2;\
						y = (y > (TYPE)0.5 ? 1 - y : y) * 2;\
						z = (z > (TYPE)0.5 ? 1 - z : z) * 2;\
						a = (a > (TYPE)0.5 ? 1 - a : a) * 2;\
					}\
					((TYPE *)(stream->buf))[4 * i + 0] = x;\
					((TYPE *)(stream->buf))[4 * i + 1] = y;\
					((TYPE *)(stream->buf))[4 * i + 2] = z;\
					((TYPE *)(stream->buf))[4 * i + 3] = a;\
				}\
			}\
			n *= stream->pixel_size;\
			ewriteall(STDOUT_FILENO, stream->buf, n, "<stdout>");\
			memmove(stream->buf, stream->buf + n, stream->ptr -= n);\
		} while (eread_stream(stream, SIZE_MAX));\
		if (stream->ptr)\
			eprintf("%s: incomplete frame\n", stream->file);\
	}

PROCESS(double, lf)
PROCESS(float, f)


int
main(int argc, char *argv[])
{
	struct stream stream;
	void (*process)(struct stream *stream);

	ARGBEGIN {
	case 'e':
		equal = 1;
		break;
	case 's':
		spiral = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc)
		usage();

	eopen_stream(&stream, NULL);

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_lf;
	else if (!strcmp(stream.pixfmt, "xyza f"))
		process = process_f;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	process(&stream);
	return 0;
}
