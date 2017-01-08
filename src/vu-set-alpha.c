/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "util.h"

#include <inttypes.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s [-i] colour-stream alpha-stream\n", argv0);
}

int
main(int argc, char *argv[])
{
	int invert = 0;
	int fd_colour = -1;
	int fd_alpha = -1;
	unsigned char buf_colour[1024];
	unsigned char buf_alpha[1024];
	size_t ptr_colour = 0;
	size_t ptr_alpha = 0;
	ssize_t r;
	size_t i, n;
	unsigned long int re, gr, bl, a1, a2;

	ARGBEGIN {
	case 'i':
		invert = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 2)
		usage();

	fd_colour = open(argv[0], O_RDONLY);
	if (fd_colour < 0)
		eprintf("open %s:", argv[0]);

	fd_alpha = open(argv[1], O_RDONLY);
	if (fd_alpha < 0)
		eprintf("open %s:", argv[1]);

	for (;;) {
		r = read(fd_colour, buf_colour + ptr_colour, sizeof(buf_colour) - ptr_colour);
		if (r < 0) {
			eprintf("read %s:", argv[0]);
		} else if (r == 0) {
			close(fd_colour);
			fd_colour = -1;
			break;
		} else {
			ptr_colour += (size_t)r;
		}

		r = read(fd_alpha, buf_alpha + ptr_alpha, sizeof(buf_alpha) - ptr_alpha);
		if (r < 0) {
			eprintf("read %s:", argv[0]);
		} else if (r == 0) {
			close(fd_alpha);
			fd_alpha = -1;
			break;
		} else {
			ptr_alpha += (size_t)r;
		}

		n = ptr_colour < ptr_alpha ? ptr_colour : ptr_alpha;
		n -= n & 3;
		ptr_colour -= n;
		ptr_alpha -= n;

		for (i = 0; i < n; i += 4) {
			re = buf_alpha[i + 0];
			gr = buf_alpha[i + 1];
			bl = buf_alpha[i + 2];
			a1 = buf_alpha[i + 3];
			a2 = buf_colour[i + 3];
			re += gr + bl;
			if (invert)
				re = 765 - re;
			a1 *= a2 * re;
			a1 = (a1 * 2 + 3 * 255 * 255) / (3 * 255 * 255 * 2);
			buf_colour[i + 3] = a1;
		}

		for (i = 0; i < n; i++) {
			r = write(STDOUT_FILENO, buf_colour + i, n - i);
			if (r < 0)
				eprintf("write <stdout>:");
			i += (size_t)r;
		}

		if ((n & 3) || ptr_colour != ptr_alpha) {
			memmove(buf_colour, buf_colour + n, ptr_colour);
			memmove(buf_alpha,  buf_alpha  + n, ptr_alpha);
		}
	}

	if (fd_alpha >= 0)
		close(fd_alpha);

	while (ptr_colour) {
		r = write(STDOUT_FILENO, buf_colour, ptr_colour);
		if (r < 0)
			eprintf("write <stdout>:");
		ptr_colour -= (size_t)r;
	}

	if (fd_colour >= 0) {
		for (;;) {
			r = read(fd_colour, buf_colour, sizeof(buf_colour));
			if (r < 0) {
				eprintf("read %s:", argv[0]);
			} else if (r == 0) {
				close(fd_colour);
				fd_colour = -1;
				break;
			} else {
				n = (size_t)r;
			}

			r = write(STDOUT_FILENO, buf_colour, n);
			if (r < 0)
				eprintf("write <stdout>:");
			n -= (size_t)r;
		}
	}

	return 0;
}
