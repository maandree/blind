/* See LICENSE file for copyright and license details. */
#ifndef TYPE
#define INCLUDE_UINT16
#include "common.h"

USAGE("")

#define FILE "blind-to-text.c"
#include "define-functions.h"

int
main(int argc, char *argv[])
{
	struct stream stream;
	void (*process)(struct stream *stream, size_t n);

	UNOFLAGS(argc);

	eopen_stream(&stream, NULL);

	SELECT_PROCESS_FUNCTION(&stream);
	printf("%zu %zu %zu %s\n", stream.frames, stream.width, stream.height, stream.pixfmt);
	process_stream(&stream, process);
	efshut(stdout, "<stdout>");
	return 0;
}

#else

static void
PROCESS(struct stream *stream, size_t n)
{
	size_t i;
	TYPE *p = (TYPE *)(stream->buf);
	for (i = 0, n /= stream->chan_size; i < n; i++)
#ifdef INTEGER_TYPE
		printf("%"PRINT_TYPE"%c", (PRINT_CAST)(p[i]), (i + 1) % stream->n_chan ? ' ' : '\n');
#else
		printf("%.25"PRINT_TYPE"%c", (PRINT_CAST)(p[i]), (i + 1) % stream->n_chan ? ' ' : '\n');
#endif
}

#endif
