/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("start-point (end-point | 'end') file")

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t frame_size, start = 0, end = 0, ptr, max, n;
	ssize_t r;
	char buf[BUFSIZ];
	int to_end = 0;

	ENOFLAGS(argc != 3);

	if (!strcmp(argv[0], "end"))
		eprintf("refusing to create video with zero frames\n");
	else
		start = etozu_arg("the start point", argv[1], 0, SIZE_MAX);

	if (!strcmp(argv[1], "end"))
		to_end = 1;
	else
		end = etozu_arg("the end point", argv[1], 0, SIZE_MAX);

	stream.file = argv[2];
	stream.fd = eopen(stream.file, O_RDONLY);
	einit_stream(&stream);
	if (to_end)
		end = stream.frames;
	else if (end > stream.frames)
		eprintf("end point is after end of video\n");
	stream.frames = end - start;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, stream.file);
	frame_size = stream.width * stream.height * stream.pixel_size;
	if (stream.frames > SSIZE_MAX / frame_size)
		eprintf("%s: video is too large\n", stream.file);

	if (start >= end)
		eprintf("%s\n", start > end ?
			"start point is after end point" :
			"refusing to create video with zero frames");
	end   *= frame_size;
	start *= frame_size;

	for (ptr = start; ptr < end;) {
		max = end - ptr;
		max = sizeof(buf) < max ? sizeof(buf) : max;
		r = read(stream.fd, buf + ptr, max);
		if (r < 0)
			eprintf("read %s:", stream.file);
		if (r == 0)
			eprintf("%s: file is shorter than expected\n", stream.file);
		ptr += n = (size_t)r;
		ewriteall(STDOUT_FILENO, buf, n, "<stdout>");
	}

	close(stream.fd);
	return 0;
}
