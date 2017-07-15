/* See LICENSE file for copyright and license details. */
#include "common.h"

#include <string.h>

USAGE("")

#define PROCESS(TYPE, CAST, FMT)\
	do {\
		size_t i;\
		TYPE *p = (TYPE *)(stream->buf);\
		for (i = 0, n /= stream->chan_size; i < n; i++)\
			printf("%"FMT"%c", (CAST)(p[i]), (i + 1) % stream->n_chan ? ' ' : '\n');\
	} while (0)

static void process_lf(struct stream *stream, size_t n) {PROCESS(double, double, ".25lf");}
static void process_f (struct stream *stream, size_t n) {PROCESS(float,  double, ".25lf");}

int
main(int argc, char *argv[])
{
	struct stream stream;
	void (*process)(struct stream *stream, size_t n);

	UNOFLAGS(argc);

	eopen_stream(&stream, NULL);

	if (stream.encoding == DOUBLE)
		process = process_lf;
	else if (stream.encoding == FLOAT)
		process = process_f;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	printf("%zu %zu %zu %s\n", stream.frames, stream.width, stream.height, stream.pixfmt);

	process_stream(&stream, process);
	efshut(stdout, "<stdout>");
	return 0;
}
