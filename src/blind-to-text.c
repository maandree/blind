/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("")

static void
process_xyza(struct stream *stream, size_t n)
{
	size_t i;
	for (i = 0; i < n; i += stream->pixel_size)
		printf("%lf %lf %lf %lf\n",
		       ((double *)(stream->buf + i))[0],
		       ((double *)(stream->buf + i))[1],
		       ((double *)(stream->buf + i))[2],
		       ((double *)(stream->buf + i))[3]);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t n;
	void (*process)(struct stream *stream, size_t n) = NULL;

	ENOFLAGS(argc);

	stream.file = "<stdin>";
	stream.fd = STDIN_FILENO;
	einit_stream(&stream);

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	printf("%zu %zu %zu %s\n", stream.frames, stream.width, stream.height, stream.pixfmt);

	while ((n = eread_stream(&stream, SIZE_MAX))) {
		n = stream.ptr - (stream.ptr % stream.pixel_size);
		process(&stream, n);
		memmove(stream.buf, stream.buf + n, stream.ptr -= n);
	}

	efshut(stdout, "<stdout>");
	return 0;
}
