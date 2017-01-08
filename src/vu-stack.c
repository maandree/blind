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

struct file
{
	int fd;
	unsigned char buf[1024];
	size_t ptr;
	const char *file;
};

static unsigned int
clip(unsigned int value)
{
	return value < 0 ? 0 : value > 255 ? 255 : value;
}

static void
blend(struct file *files, size_t n_files, size_t n)
{
	double r1, g1, b1, a1;
	double r2, g2, b2, a2;
	size_t i, j;
	for (i = 0; i < n; i += 4) {
		r1 = srgb_decode(files->buf[i + 0] / 255.);
		g1 = srgb_decode(files->buf[i + 1] / 255.);
		b1 = srgb_decode(files->buf[i + 2] / 255.);
		a1 = files->buf[i + 3] / 255.;
		for (j = 1; j < n_files;) {
			r2 = srgb_decode(files[j].buf[i + 0] / 255.);
			g2 = srgb_decode(files[j].buf[i + 1] / 255.);
			b2 = srgb_decode(files[j].buf[i + 2] / 255.);
			a2 = files[j].buf[i + 3] / 255.;
			a2 /= ++j;
			r1 = r1 * a1 * (1 - a2) + r2 * a2;
			g1 = g1 * a1 * (1 - a2) + g2 * a2;
			b1 = b1 * a1 * (1 - a2) + b2 * a2;
			a1 = a1 * (1 - a2) + a2;
		}
		r1 = srgb_encode(r1) * 255 + 0.5;
		g1 = srgb_encode(g1) * 255 + 0.5;
		b1 = srgb_encode(b1) * 255 + 0.5;
		a1 = a1 * 255 * 0.5;
		files->buf[i + 0] = clip((unsigned int)r1);
		files->buf[i + 1] = clip((unsigned int)g1);
		files->buf[i + 2] = clip((unsigned int)b1);
		files->buf[i + 3] = clip((unsigned int)a1);
	}
}

static void
stack(struct file *files, size_t n_files, size_t n)
{
	double r1, g1, b1, a1;
	double r2, g2, b2, a2;
	size_t i, j;
	for (i = 0; i < n; i += 4) {
		r1 = srgb_decode(files->buf[i + 0] / 255.);
		g1 = srgb_decode(files->buf[i + 1] / 255.);
		b1 = srgb_decode(files->buf[i + 2] / 255.);
		a1 = files->buf[i + 3] / 255.;
		for (j = 1; j < n_files; j++) {
			r2 = srgb_decode(files[j].buf[i + 0] / 255.);
			g2 = srgb_decode(files[j].buf[i + 1] / 255.);
			b2 = srgb_decode(files[j].buf[i + 2] / 255.);
			a2 = files[j].buf[i + 3] / 255.;
			r1 = r1 * a1 * (1 - a2) + r2 * a2;
			g1 = g1 * a1 * (1 - a2) + g2 * a2;
			b1 = b1 * a1 * (1 - a2) + b2 * a2;
			a1 = a1 * (1 - a2) + a2;
		}
		r1 = srgb_encode(r1) * 255 + 0.5;
		g1 = srgb_encode(g1) * 255 + 0.5;
		b1 = srgb_encode(b1) * 255 + 0.5;
		a1 = a1 * 255 * 0.5;
		files->buf[i + 0] = clip((unsigned int)r1);
		files->buf[i + 1] = clip((unsigned int)g1);
		files->buf[i + 2] = clip((unsigned int)b1);
		files->buf[i + 3] = clip((unsigned int)a1);
	}
}

static void
usage(void)
{
	eprintf("usage: %s [-b] bottom-image ... top-image\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct file *files;
	size_t n_files;
	int blend_flag = 0;
	size_t i, j, n;
	ssize_t r;
	size_t closed;

	ARGBEGIN {
	case 'b':
		blend_flag = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc < 2)
		usage();

	n_files = (size_t)argc;
	files = calloc(n_files, sizeof(*files));
	if (!files)
		eprintf("calloc:");

	for (i = 0; i < n_files; i++) {
		files[i].fd = open(argv[i], O_RDONLY);
		if (files[i].fd < 0)
			eprintf("open %s:", argv[i]);
		files[i].file = argv[i];
	}

	while (n_files) {
		n = SIZE_MAX;
		for (i = 0; i < n_files; i++) {
			r = read(files[i].fd, files[i].buf + files[i].ptr, sizeof(files[i].buf) - files[i].ptr);
			if (r < 0) {
				eprintf("read %s:", files[i].file);
			} else if (r == 0) {
				close(files[i].fd);
				files[i].fd = -1;
			} else {
				files[i].ptr += (size_t)r;
			}
			if (files[i].ptr && files[i].ptr < n)
				n = files[i].ptr;
		}
		n -= n & 3;

		(blend_flag ? blend : stack)(files, n_files, n);
		for (j = 0; j < n;) {
			r = write(STDOUT_FILENO, files->buf + j, n - j);
			if (r < 0)
				eprintf("write <stdout>:");
			j += (size_t)r;
		}

		closed = SIZE_MAX;
		for (i = 0; i < n_files; i++) {
			memmove(files[i].buf, files[i].buf + n, files[i].ptr -= n);
			if (files[i].ptr < 4 && files[i].fd < 0 && closed == SIZE_MAX)
				closed = i;
		}
		if (closed != SIZE_MAX) {
			for (i = (j = closed) + 1; i < n_files; i++)
				if (files[i].ptr < 4 && files[i].fd < 0)
					files[j++] = files[i];
			n_files = j;
		}
	}

	return 0;
}
