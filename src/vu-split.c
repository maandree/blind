/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <alloca.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

USAGE("(file (end-point | 'end')) ...")

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t *ends, i, parts, ptr, end, frame_size, n;
	FILE *fp;
	int fd;

	ENOFLAGS(argc < 2 || argc % 2);

	stream.file = "<stdin>";
	stream.fd = STDIN_FILENO;
	einit_stream(&stream);
	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, stream.file);
	frame_size = stream.width * stream.height * stream.pixel_size;
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
		fd = eopen(argv[i * 2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
		fp = fdopen(fd, "wb");
		if (!fp)
			eprintf("fdopen %s:", argv[i * 2]);

		stream.frames = ends[i] - (i ? ends[i - 1] : 0);
		fprint_stream_head(fp, &stream);
		efflush(fp, argv[i * 2]);

		for (end = ends[i] * frame_size; ptr < end; ptr += n) {
			if (stream.ptr == sizeof(stream.buf))
				n = stream.ptr < end - ptr ? stream.ptr : end - ptr;
			else if (!(n = eread_stream(&stream, end - ptr)))
				eprintf("%s: file is shorter than expected\n", stream.file);
			ewriteall(STDOUT_FILENO, stream.buf, n, "<stdout>");
			memmove(stream.buf, stream.buf + n, stream.ptr -= n);
		}

		if (fclose(fp))
			eprintf("%s:", argv[i * 2]);
		close(fd);
	}

	return 0;
}
