/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s count file\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t count, ptr, n, ptw;
	ssize_t r;
	char buf[BUFSIZ];

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	if (argc != 2)
		usage();

	if (tozu(argv[0], 0, SIZE_MAX, &count))
		eprintf("the count must be an integer in [0, %zu]\n", SIZE_MAX);

	stream.file = argv[1];
	stream.fd = open(stream.file, O_RDONLY);
	if (stream.fd < 0)
		eprintf("open %s:", stream.file);
	einit_stream(&stream);
	if (stream->frame > SIZE_MAX / count)
		eprintf("%s: video too long\n", stream.file);
	stream->frame *= count;
	fprint_stream_head(stdout, &stream);
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");

	while (count--) {
		for (ptw = 0; ptw < stream.ptr;) {
			r = write(STDOUT_FILENO, stream.buf + ptw, stream.ptr - ptw);
			if (r < 0)
				eprintf("write <stdout>:");
			ptw += (size_t)r;
		}
		for (ptr = 0;;) {
			r = pread(stream.fd, buf, sizeof(buf), ptr);
			if (r < 0)
				eprintf("pread %s:", argv[1]);
			else if (r == 0)
				break;
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
