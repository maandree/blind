/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s first-stream ... last-stream\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stream stream, refstream;
	size_t ptr;
	ssize_t r;
	int i;

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	if (argc < 2)
		usage();

	for (i = 0; i < argc; i++) {
		stream.file = argv[i];
		stream.fd = open(stream.file, O_RDONLY);
		if (stream.fd < 0)
			eprintf("open %s:", stream.file);
		einit_stream(&stream);

		if (!i) {
			memcpy(&refstream, &stream, sizeof(stream));
			fprint_stream_head(stdout, &stream);
			fflush(stdout);
			if (ferror(stdout))
				eprintf("<stdout>:");
		} else {
			if (stream.width != refstream.width || stream.height != refstream.height)
				eprintf("videos do not have the same geometry\n");
			if (strcmp(stream.pixfmt, refstream.pixfmt))
				eprintf("videos use incompatible pixel formats\n");
		}

		for (; eread_stream(&stream, SIZE_MAX); stream.ptr = 0) {
			for (ptr = 0; ptr < stream.ptr; ptr += (size_t)r) {
				r = write(STDOUT_FILENO, stream.buf + ptr, stream.ptr - ptr);
				if (r < 0)
					eprintf("write <stdout>");
			}
		}

		close(stream.fd);
	}

	return 0;
}
