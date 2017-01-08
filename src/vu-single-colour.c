/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "util.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s [-f frames] -w width -h height red green blue [alpha]\n", argv0);
}

int
main(int argc, char *argv[])
{
	int red, green, blue, alpha = 255;
	size_t width = 0, height = 0, frames = 1;
	unsigned char pixel[4];
	size_t x, y, n;
	int32_t buf[1024];
	ssize_t r;

	ARGBEGIN {
	case 'f':
		if (tozu(EARGF(usage()), 1, SIZE_MAX, &frames))
			eprintf("argument of -f must be an integer in [1, %zu]\n", SIZE_MAX);
		break;
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

	if (!width || !height || argc < 3 || argc > 4)
		usage();

	if (toi(argv[0], 0, 255, &red))
		eprintf("the red value must be an integer in [0, 255]\n");
	if (toi(argv[1], 0, 255, &green))
		eprintf("the green value must be an integer in [0, 255]\n");
	if (toi(argv[2], 0, 255, &blue))
		eprintf("the blue value must be an integer in [0, 255]\n");
	if (argc > 3 && toi(argv[3], 0, 255, &alpha))
		eprintf("the alpha value must be an integer in [0, 255]\n");

	pixel[0] = (unsigned char)red;
	pixel[1] = (unsigned char)green;
	pixel[2] = (unsigned char)blue;
	pixel[3] = (unsigned char)alpha;

	for (x = 0; x < ELEMENTSOF(buf); x++)
		buf[x] = *(int32_t *)(void *)pixel;
	while (frames--) {
		for (y = height; y--;) {
			for (x = width; x;) {
				n = ELEMENTSOF(buf) < x ? ELEMENTSOF(buf) : x;
				x -= n;
				n *= sizeof(*buf);
				while (n) {
					r = write(STDOUT_FILENO, buf, n);
					if (r < 0)
						eprintf("write <stdout>:");
					n -= (size_t)r;
				}
			}
		}
	}

	efshut(stdout, "<stdout>");
	return 0;
}
