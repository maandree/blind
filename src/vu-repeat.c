/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s (count | 'inf') file\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t count = 0, ptr, n, ptw;
	ssize_t r;
	char buf[BUFSIZ];
	int inf = 0;

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	if (argc != 2)
		usage();

	if (!strcmp(argv[0], "inf")) {
		inf = 1;
	} else if (tozu(argv[0], 0, SIZE_MAX, &count)) {
		eprintf("the count must be an integer in [0, %zu]\n", SIZE_MAX);
	}

	if (inf)
		einf_check_fd(STDOUT_FILENO, "<stdout>");

	stream.file = argv[1];
	stream.fd = open(stream.file, O_RDONLY);
	if (stream.fd < 0)
		eprintf("open %s:", stream.file);
	einit_stream(&stream);
	if (count > SIZE_MAX / stream.frames)
		eprintf("%s: video too long\n", stream.file);
	stream.frames *= count;
	fprint_stream_head(stdout, &stream);
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");

	while (inf || count--) {
		posix_fadvise(stream.fd, 0, 0, POSIX_FADV_SEQUENTIAL);
		for (ptw = 0; ptw < stream.ptr;) {
			r = write(STDOUT_FILENO, stream.buf + ptw, stream.ptr - ptw);
			if (r < 0)
				goto writeerr;
			ptw += (size_t)r;
		}
		for (ptr = 0;;) {
			r = pread(stream.fd, buf, sizeof(buf), ptr);
			if (r < 0)
				eprintf("pread %s:", stream.file);
			else if (r == 0)
				break;
			ptr += n = (size_t)r;
			for (ptw = 0; ptw < n;) {
				r = write(STDOUT_FILENO, buf + ptw, n - ptw);
				if (r < 0)
					goto writeerr;
				ptw += (size_t)r;
			}
		}
	}

	close(stream.fd);
	return 0;

writeerr:
	if (!inf || errno != EPIPE)
		eprintf("write <stdout>:");
	return 0;
}
