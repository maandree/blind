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
	struct stream *streams;
	size_t ptr, frames = 0;
	ssize_t r;
	int i;

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	if (argc < 2)
		usage();

	streams = malloc((size_t)argc * sizeof(*streams));
	if (!streams)
		eprintf("malloc:");

	for (i = 0; i < argc; i++) {
		streams[i].file = argv[i];
		streams[i].fd = open(streams[i].file, O_RDONLY);
		if (streams[i].fd < 0)
			eprintf("open %s:", streams[i].file);
		einit_stream(streams + i);

		if (i) {
			if (streams[i].width != streams->width || streams[i].height != streams->height)
				eprintf("videos do not have the same geometry\n");
			if (strcmp(streams[i].pixfmt, streams->pixfmt))
				eprintf("videos use incompatible pixel formats\n");
		}

		if (streams[i].frames > SIZE_MAX - frames)
			eprintf("resulting video is too long\n");
		frames += streams[i].frames;
	}

	streams->frames = frames;
	fprint_stream_head(stdout, streams);
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");

	for (i = 0; i < argc; i++) {
		for (; eread_stream(streams + i, SIZE_MAX); streams[i].ptr = 0) {
			for (ptr = 0; ptr < streams[i].ptr; ptr += (size_t)r) {
				r = write(STDOUT_FILENO, streams[i].buf + ptr, streams[i].ptr - ptr);
				if (r < 0)
					eprintf("write <stdout>");
			}
		}
		close(streams[i].fd);
	}

	return 0;
}
