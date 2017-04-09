/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

USAGE("[-b] bottom-stream ... top-stream")

#define PROCESS_LINEAR_3CH_ALPHA(TYPE, BLEND)\
	do {\
		TYPE x1, y1, z1, a1;\
		TYPE x2, y2, z2, a2;\
		size_t i, j;\
		for (i = 0; i < n; i += streams->pixel_size) {\
			x1 = ((TYPE *)(streams[0].buf + i))[0];\
			y1 = ((TYPE *)(streams[0].buf + i))[1];\
			z1 = ((TYPE *)(streams[0].buf + i))[2];\
			a1 = ((TYPE *)(streams[0].buf + i))[3];\
			for (j = 1; j < n_streams; j++) {\
				x2 = ((TYPE *)(streams[j].buf + i))[0];\
				y2 = ((TYPE *)(streams[j].buf + i))[1];\
				z2 = ((TYPE *)(streams[j].buf + i))[2];\
				a2 = ((TYPE *)(streams[j].buf + i))[3];\
				if (BLEND)\
					a2 /= j + 1;\
				x1 = x1 * a1 * (1 - a2) + x2 * a2;\
				y1 = y1 * a1 * (1 - a2) + y2 * a2;\
				z1 = z1 * a1 * (1 - a2) + z2 * a2;\
				a1 = a1 * (1 - a2) + a2;\
			}\
			((TYPE *)(streams[0].buf + i))[0] = x1;\
			((TYPE *)(streams[0].buf + i))[1] = y1;\
			((TYPE *)(streams[0].buf + i))[2] = z1;\
			((TYPE *)(streams[0].buf + i))[3] = a1;\
		}\
	} while (0)

static void process_xyza  (struct stream *streams, size_t n_streams, size_t n) { PROCESS_LINEAR_3CH_ALPHA(double, 0); }
static void process_xyza_b(struct stream *streams, size_t n_streams, size_t n) { PROCESS_LINEAR_3CH_ALPHA(double, 1); }

int
main(int argc, char *argv[])
{
	struct stream *streams;
	size_t n_streams, i, frames = 0, tmp;
	int blend = 0;
	void (*process)(struct stream *streams, size_t n_streams, size_t n) = NULL;

	ARGBEGIN {
	case 'b':
		blend = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc < 2)
		usage();

	n_streams = (size_t)argc;
	streams = ecalloc(n_streams, sizeof(*streams));

	for (i = 0; i < n_streams; i++) {
		eopen_stream(streams + i, argv[i]);
		if (streams[i].frames > frames)
			frames = streams[i].frames;
	}

	if (!strcmp(streams->pixfmt, "xyza"))
		process = blend ? process_xyza_b : process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", streams->pixfmt);

	tmp = streams->frames, streams->frames = frames;
	fprint_stream_head(stdout, streams);
	efflush(stdout, "<stdout>");
	streams->frames = tmp;
	process_multiple_streams(streams, n_streams, STDOUT_FILENO, "<stdout>", process);

	free(streams);
	return 0;
}
