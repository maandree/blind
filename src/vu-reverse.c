/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s file\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t frame_size, ptr, end, n, ptw;
	ssize_t r;
	char buf[BUFSIZ];

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	if (argc != 1)
		usage();

	stream.file = argv[1];
	stream.fd = open(stream.file, O_RDONLY);
	if (stream.fd < 0)
		eprintf("open %s:", stream.file);
	einit_stream(&stream);
	fprint_stream_head(stdout, &stream);
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");
	if (stream.width > SIZE_MAX / stream.height)
		eprintf("%s: video is too large\n", stream.file);
	frame_size = stream.width * stream.height;
	if (stream.frames > SSIZE_MAX / frame_size)
		eprintf("%s: video is too large\n", stream.file);

	posix_fadvise(stream.fd, 0, 0, POSIX_FADV_RANDOM);
	while (stream.frames--) {
		ptr = stream.frames * frame_size;
		end = ptr + frame_size;
		while (ptr < end) {
			r = pread(stream.fd, buf, sizeof(buf), ptr);
			if (r < 0)
				eprintf("pread %s:", stream.file);
			else if (r == 0)
				eprintf("%s: file is shorter than expected", stream.file);
			ptr += n = (size_t)r;
			for (ptw = 0; ptw < n;) {
				r = write(STDOUT_FILENO, buf + ptw, n - ptw);
				if (r < 0)
					eprintf("write <stdout>:");
				ptw += (size_t)r;
			}
		}
	}

	close(stream.fd);
	return 0;
}
