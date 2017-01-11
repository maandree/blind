/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("(count | 'inf') file")

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t count = 0, ptr, n;
	ssize_t r;
	char buf[BUFSIZ];
	int inf = 0;

	ENOFLAGS(argc != 2);

	if (!strcmp(argv[0], "inf"))
		inf = 1;
	else
		count = etozu_arg("the count", argv[0], 0, SIZE_MAX);

	if (inf)
		einf_check_fd(STDOUT_FILENO, "<stdout>");

	stream.file = argv[1];
	stream.fd = eopen(stream.file, O_RDONLY);
	einit_stream(&stream);
	if (count > SIZE_MAX / stream.frames)
		eprintf("%s: video is too long\n", stream.file);
	stream.frames *= count;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	while (inf || count--) {
		posix_fadvise(stream.fd, 0, 0, POSIX_FADV_SEQUENTIAL);
		if (writeall(STDOUT_FILENO, stream.buf, stream.ptr))
			goto writeerr;
		for (ptr = 0;;) {
			r = pread(stream.fd, buf, sizeof(buf), ptr);
			if (r < 0)
				eprintf("pread %s:", stream.file);
			else if (r == 0)
				break;
			ptr += n = (size_t)r;
			if (writeall(STDOUT_FILENO, buf, n))
				goto writeerr;
		}
	}

	close(stream.fd);
	return 0;

writeerr:
	if (!inf || errno != EPIPE)
		eprintf("write <stdout>:");
	return 0;
}
