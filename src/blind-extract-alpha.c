/* See LICENSE file for copyright and license details. */
#ifndef TYPE
#include "common.h"

USAGE("colour-file")

#define FILE "blind-extract-alpha.c"
#include "define-functions.h"

int
main(int argc, char *argv[])
{
	struct stream stream;
	int fd;
	void (*process)(struct stream *stream, int fd, const char *fname);

	UNOFLAGS(argc != 1);

	eopen_stream(&stream, NULL);
	fd = eopen(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);

	if (stream.encoding == DOUBLE)
		process = process_lf;
	else if (stream.encoding == FLOAT)
		process = process_f;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	if (dprint_stream_head(fd, &stream) < 0)
		eprintf("dprintf %s:", argv[0]);

	process(&stream, fd, argv[0]);
	return 0;
}

#else

static void
PROCESS(struct stream *stream, int fd, const char *fname)
{
	char buf[sizeof(stream->buf)];
	size_t i, j, n;
	TYPE a, *p, *b;
	do {
		n = stream->ptr / stream->pixel_size;
		p = (TYPE *)(stream->buf) + stream->n_chan - 1;
		b = (TYPE *)buf;
		for (i = 0; i < n; i++, p += stream->n_chan) {
			a = *p, *p = 1;
			for (j = stream->n_chan - 1; j--;)
				*b++ = a;
			*b++ = 1;
		}
		n *= stream->pixel_size;
		ewriteall(fd, stream->buf, n, fname);
		ewriteall(STDOUT_FILENO, buf, n, "<stdout>");
		memmove(stream->buf, stream->buf + n, stream->ptr -= n);
	} while (eread_stream(stream, SIZE_MAX));
	if (stream->ptr)
		eprintf("%s: incomplete frame\n", stream->file);
}

#endif
