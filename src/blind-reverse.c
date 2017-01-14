/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <limits.h>
#include <unistd.h>

USAGE("file")

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t frame_size, ptr, end, n;
	ssize_t r;
	char buf[BUFSIZ];

	ENOFLAGS(argc != 1);

	stream.file = argv[0];
	stream.fd = eopen(stream.file, O_RDONLY);
	einit_stream(&stream);
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, stream.file);
	frame_size = stream.width * stream.height * stream.pixel_size;
	if (stream.frames > SSIZE_MAX / frame_size)
		eprintf("%s: video is too large\n", stream.file);
	if (stream.frames * frame_size > SSIZE_MAX - stream.headlen)
		eprintf("%s: video is too large\n", stream.file);

#if defined(POSIX_FADV_RANDOM)
	posix_fadvise(stream.fd, 0, 0, POSIX_FADV_RANDOM);
#endif
	while (stream.frames--) {
		ptr = stream.frames * frame_size + stream.headlen;
		end = ptr + frame_size;
		while (ptr < end) {
			r = pread(stream.fd, buf, sizeof(buf), ptr);
			if (r < 0)
				eprintf("pread %s:", stream.file);
			else if (r == 0)
				eprintf("%s: file is shorter than expected\n", stream.file);
			ptr += n = (size_t)r;
			ewriteall(STDOUT_FILENO, buf, n, "<stdout>");
		}
	}

	close(stream.fd);
	return 0;
}
