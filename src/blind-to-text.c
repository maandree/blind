/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <string.h>

USAGE("")

#define PROCESS(TYPE, CAST, FMT)\
	do {\
		size_t i;\
		for (i = 0; i < n; i += stream->pixel_size)\
			printf("%"FMT" %"FMT" %"FMT" %"FMT"\n",\
			       (CAST)(((TYPE *)(stream->buf + i))[0]),\
			       (CAST)(((TYPE *)(stream->buf + i))[1]),\
			       (CAST)(((TYPE *)(stream->buf + i))[2]),\
			       (CAST)(((TYPE *)(stream->buf + i))[3]));\
	} while (0)

static void process_xyza (struct stream *stream, size_t n) {PROCESS(double, double, "lf");}
static void process_xyzaf(struct stream *stream, size_t n) {PROCESS(float,  double, "lf");}

int
main(int argc, char *argv[])
{
	struct stream stream;
	void (*process)(struct stream *stream, size_t n);

	UNOFLAGS(argc);

	eopen_stream(&stream, NULL);

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_xyza;
	else if (!strcmp(stream.pixfmt, "xyza f"))
		process = process_xyzaf;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	printf("%zu %zu %zu %s\n", stream.frames, stream.width, stream.height, stream.pixfmt);

	process_stream(&stream, process);
	efshut(stdout, "<stdout>");
	return 0;
}
