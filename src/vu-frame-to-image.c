/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "util.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s -w width -h height\n", argv0);
}

int
main(int argc, char *argv[])
{
	size_t width = 0, height = 0;
	size_t p, n;
	ssize_t r;
	char buf[8096];

	ARGBEGIN {
	case 'w':
		if (tozu(EARGF(usage()), 1, SIZE_MAX, &width))
			eprintf("argument of -w must be an integer in [1, %zu]\n", SIZE_MAX);
		break;
	case 'h':
		if (tozu(EARGF(usage()), 1, SIZE_MAX, &height))
			eprintf("argument of -h must be an integer in [1, %zu]\n", SIZE_MAX);
		break;
	default:
		usage();
	} ARGEND;

	if (!width || !height || argc)
		usage();

	printf("P7\n"
	       "WIDTH %zu\n"
	       "HEIGHT %zu\n"
	       "DEPTH 4\n"
	       "MAXVAL 255\n"
	       "TUPLTYPE RGB_ALPHA\n"
	       "ENDHDR\n", width, height);
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");

	for (n = 0;;) {
		for (p = 0; p < n;) {
			r = write(STDOUT_FILENO, buf + p, n - p);
			if (r < 0)
				eprintf("write <stdin>:");
			p += (size_t)r;
		}
		r = read(STDIN_FILENO, buf, sizeof(buf));
		if (r < 0)
			 eprintf("read <stdin>:");
		if (r == 0)
			break;
		n = (size_t)r;
	}

	return 0;
}
