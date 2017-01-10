/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <alloca.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s (file end-point) ...\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t *ends, i, parts, ptr, end, frame_size, max, ptw, n;
	FILE *fp;
	int fd;
	ssize_t r;

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	if (argc < 2 || argc % 2)
		usage();

	stream.file = "<stdin>";
	stream.fd = STDIN_FILENO;
	einit_stream(&stream);
	if (stream.width > SIZE_MAX / stream.height)
		eprintf("%s: video is too large\n", stream.file);
	frame_size = stream.width * stream.height;
	if (stream.frames > SSIZE_MAX / frame_size)
		eprintf("%s: video is too large\n", stream.file);

	parts = (size_t)argc / 2;
	ends = alloca(parts * sizeof(*ends));

	for (i = 0; i < parts; i++) {
		if (!strcmp(argv[i * 2 + 1], "end"))
			ends[i] = stream.frames;
		else if (tozu(argv[i * 2 + 1], 0, SIZE_MAX, ends + i))
			eprintf("the end point must be an integer in [0, %zu]\n", SIZE_MAX);
		if (i && ends[i] <= ends[i - 1])
			eprintf("the end points must be in strictly ascending order\n");
		if (ends[i] > stream.frames)
			eprintf("frame %zu is beyond the end of the video\n", ends[i]);
	}

	ptr = 0;
	for (i = 0; i < parts; i++) {
		fd = open(argv[i * 2], O_WRONLY);
		if (fd < 0)
			eprintf("open %s:", argv[i * 2]);
		fp = fdopen(fd, "wb");

		stream.frames = ends[i] - (i ? ends[i - 1] : 0);
		fprint_stream_head(fp, &stream);
		fflush(fp);
		if (ferror(fp))
			eprintf("%s:", argv[i * 2]);

		for (end = ends[i] * frame_size; ptr < end; ptr += n) {
			for (ptw = ptr; stream.ptr && ptw < end;) {
				max = end - ptw;
				max = max < stream.ptr ? max : stream.ptr;
				r = write(fd, stream.buf, max);
				if (r < 0)
					eprintf("write %s:\n", argv[i * 2]);
				memmove(stream.buf, stream.buf + r, stream.ptr - (size_t)r);
			}
			n = eread_stream(&stream, end - ptr);
			if (n == 0)
				eprintf("%s: file is shorter than expected\n", stream.file);
		}

		if (fclose(fp))
			eprintf("%s:", argv[i * 2]);
		close(fd);
	}

	return 0;
}
