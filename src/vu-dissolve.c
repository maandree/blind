/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "util.h"

#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static unsigned int
clip(unsigned int value)
{
	return value > 255 ? 255 : value;
}

static void
usage(void)
{
	eprintf("usage: %s [-r] -f frames -w width -h height\n", argv0);
}

int
main(int argc, char *argv[])
{
	int reverse = 0;
	size_t frames = 0, width = 0, height = 0;
	unsigned char buf[8096];
	size_t frame, h, w, max;
	size_t ptr = 0, n, i, fm;
	ssize_t r;
	double a;

	ARGBEGIN {
	case 'f':
		if (tozu(EARGF(usage()), 2, SIZE_MAX, &frames))
			eprintf("argument of -f must be an integer in [2, %zu]\n", SIZE_MAX);
		break;
	case 'w':
		if (tozu(EARGF(usage()), 1, SIZE_MAX, &width))
			eprintf("argument of -w must be an integer in [1, %zu]\n", SIZE_MAX);
		break;
	case 'h':
		if (tozu(EARGF(usage()), 1, SIZE_MAX, &height))
			eprintf("argument of -h must be an integer in [1, %zu]\n", SIZE_MAX);
		break;
	case 'r':
		reverse = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc || !frames || !width || !height)
		usage();

	fm = frames - 1;
	for (frame = 0; frame < frames; frame++) {
		for (h = height; h--;) {
			for (w = width << 2; w; w -= n) {
				max = sizeof(buf) - ptr < w ? sizeof(buf) - ptr : w;
				r = read(STDIN_FILENO, buf + ptr, max);
				if (r < 0)
					eprintf("read <stdin>:");
				if (r == 0)
					eprintf("content is shorter than specified\n");
				n = ptr += (size_t)r;
				n -= n & 3;
				if (reverse) {
					for (i = 0; i < n; i += 4) {
						a = buf[i + 3];
						a = a * (double)frame / fm;
						a += 0.5;
						buf[i + 3] = clip((unsigned int)a);
					}
				} else {
					for (i = 0; i < n; i += 4) {
						a = buf[i + 3];
						a = a * (double)(fm - frame) / fm;
						a += 0.5;
						buf[i + 3] = clip((unsigned int)a);
					}
				}
				for (i = 0; i < n; i += (size_t)r) {
					r = write(STDOUT_FILENO, buf + i, n - i);
					if (r < 0)
						eprintf("write <stdout>:");
				}
				memmove(buf, buf + n, ptr -= n);
			}
		}
	}

	return 0;
}
