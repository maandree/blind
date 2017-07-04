/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("colour-file")

#define PROCESS(TYPE, SUFFIX)\
	static void\
	process_##SUFFIX(struct stream *stream, int fd, const char *fname)\
	{\
		char buf[sizeof(stream->buf)];\
		size_t i, n;\
		TYPE a;\
		do {\
			n = stream->ptr / stream->pixel_size;\
			for (i = 0; i < n; i++) {\
				a = ((TYPE *)(stream->buf))[4 * i + 3];\
				((TYPE *)(stream->buf))[4 * i + 3] = 1;\
				((TYPE *)buf)[4 * i + 0] = a;\
				((TYPE *)buf)[4 * i + 1] = a;\
				((TYPE *)buf)[4 * i + 2] = a;\
				((TYPE *)buf)[4 * i + 3] = 1;\
			}\
			n *= stream->pixel_size;\
			ewriteall(fd, stream->buf, n, fname);\
			ewriteall(STDOUT_FILENO, buf, n, "<stdout>");\
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
	int fd;
	void (*process)(struct stream *stream, int fd, const char *fname);

	UNOFLAGS(argc != 1);

	eopen_stream(&stream, NULL);
	fd = eopen(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_lf;
	else if (!strcmp(stream.pixfmt, "xyza f"))
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
