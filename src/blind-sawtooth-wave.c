/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("[-e]")

static int equal = 0;

#define PROCESS(TYPE, SUFFIX)\
	static void\
	process_##SUFFIX(struct stream *stream)\
	{\
		size_t i, j, n;\
		TYPE v, *p;\
		do {\
			if (equal) {\
				n = stream->ptr / stream->pixel_size;\
				for (i = 0; i < n; i++) {\
					p = (TYPE *)(stream->buf) + i * stream->n_chan;\
					v = posmod(*p, (TYPE)1);\
					for (j = 0; j < stream->n_chan; j++)\
						p[j] = v;\
				}\
				n *= stream->pixel_size;\
			} else {\
				n = stream->ptr / stream->chan_size;\
				for (i = 0; i < n; i++) {\
					p = (TYPE *)(stream->buf) + i;\
					*p = posmod(*p, (TYPE)1);\
				}\
				n *= stream->chan_size;\
			}\
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
	default:
		usage();
	} ARGEND;

	if (argc)
		usage();

	eopen_stream(&stream, NULL);

	if (stream.encoding == DOUBLE)
		process = process_lf;
	else if (stream.encoding == FLOAT)
		process = process_f;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	process(&stream);
	return 0;
}
