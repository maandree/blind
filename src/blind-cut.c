/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("start-point (end-point | 'end') file")

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t frame_size, start = 0, end = 0, ptr, max;
	ssize_t r;
	char buf[BUFSIZ];
	int to_end = 0;

	UNOFLAGS(argc != 3);

	if (!strcmp(argv[0], "end"))
		eprintf("refusing to create video with zero frames\n");
	else
		start = etozu_arg("the start point", argv[0], 0, SIZE_MAX);

	if (!strcmp(argv[1], "end"))
		to_end = 1;
	else
		end = etozu_arg("the end point", argv[1], 0, SIZE_MAX);

	eopen_stream(&stream, argv[2]);
	if (to_end)
		end = stream.frames;
	else if (end > stream.frames)
		eprintf("end point is after end of video\n");
	stream.frames = end - start;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, stream.file);
	frame_size = stream.width * stream.height * stream.pixel_size;
	if (stream.frames > (size_t)SSIZE_MAX / frame_size)
		eprintf("%s: video is too large\n", stream.file);

	if (start >= end)
		eprintf("%s\n", start > end ?
			"start point is after end point" :
			"refusing to create video with zero frames");
	end   = end   * frame_size + stream.headlen;
	start = start * frame_size + stream.headlen;

	fadvise_sequential(stream.fd, start, end - start);
	for (ptr = start; ptr < end; ptr += (size_t)r) {
		max = end - ptr;
		max = MIN(max, sizeof(buf));
		if (!(r = epread(stream.fd, buf, max, ptr, stream.file)))
			eprintf("%s: file is shorter than expected\n", stream.file);
		ewriteall(STDOUT_FILENO, buf, (size_t)r, "<stdout>");
	}

	close(stream.fd);
	return 0;
}
