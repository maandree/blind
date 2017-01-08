/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "util.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define eprintf(...) enprintf(2, __VA_ARGS__)

static void
usage(void)
{
	eprintf("usage: %s -w width -h height\n", argv0);
}

int
main(int argc, char *argv[])
{
	size_t width = 0, height = 0;
	size_t p, n, w, max;
	ssize_t r;
	char buf[8096];
	int anything = 0;

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

	while (height) {
		height--;
		for (w = width << 2; w;) {
			max = sizeof(buf) < w ? sizeof(buf) : w;
			r = read(STDIN_FILENO, buf, max);
			if (r < 0)
				eprintf("read <stdin>:");
			if (r == 0)
				goto done;
			anything = 1;
			w -= n = (size_t)r;
			for (p = 0; p < n;) {
				r = write(STDOUT_FILENO, buf + p, n - p);
				if (r < 0)
					eprintf("write <stdin>:");
				p += (size_t)r;
			}
		}
	}
done:

	if (height || w)
		eprintf("incomplete frame\n");

	return !anything;
}
