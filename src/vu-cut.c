/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s start-point (end-point | 'end') file\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t frame_size, start = 0, end = 0, ptr, max, n, ptw;
	ssize_t r;
	char buf[BUFSIZ];
	int to_end = 0;

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	if (argc != 3)
		usage();

	if (!strcmp(argv[0], "end")) {
		eprintf("refusing to create video with zero frames\n");
	} else if (tozu(argv[0], 0, SIZE_MAX, &start)) {
		eprintf("the start point must be an integer in [0, %zu]\n", SIZE_MAX);
	}

	if (!strcmp(argv[1], "end")) {
		to_end = 1;
	} else if (tozu(argv[1], 0, SIZE_MAX, &end)) {
		eprintf("the end point must be an integer in [0, %zu]\n", SIZE_MAX);
	}

	stream.file = argv[2];
	stream.fd = open(stream.file, O_RDONLY);
	if (stream.fd < 0)
		eprintf("open %s:", stream.file);
	einit_stream(&stream);
	if (to_end)
		end = stream.frames;
	else if (end > stream.frames)
		eprintf("end point is after end of video\n");
	stream.frames = end - start;
	fprint_stream_head(stdout, &stream);
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");
	if (stream.width > SIZE_MAX / stream.height)
		eprintf("%s: video is too large\n", stream.file);
	frame_size = stream.width * stream.height;
	if (stream.frames > SSIZE_MAX / frame_size)
		eprintf("%s: video is too large\n", stream.file);

	if (start >= end)
		eprintf("%s\n", start > end ?
			"start point is after end point" :
			"refusing to create video with zero frames");
	end *= frame_size;
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
		for (ptw = 0; ptw < n; ptw += (size_t)r) {
			r = write(STDOUT_FILENO, buf + ptw, n - ptw);
			if (r < 0)
				eprintf("write <stdout>:");
		}
	}

	close(stream.fd);
	return 0;
}
