/* See LICENSE file for copyright and license details. */
#include "arg.h"
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
	struct stat st;
	size_t count, ptr, n, ptw;
	int fd;
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

	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		eprintf("open %s:", argv[1]);

	while (count--) {
		posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
		for (ptr = 0;;) {
			r = pread(fd, buf, sizeof(buf), ptr);
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

	close(fd);
	return 0;
}
