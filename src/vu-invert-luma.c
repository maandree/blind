/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s [-i] mask-stream\n", argv0);
}

static void
process_xyza(struct stream *colour, struct stream *mask, size_t n)
{
	size_t i;
	double w, y;
	for (i = 0; i < n; i += colour->pixel_size) {
		w = ((double *)(mask->buf + i))[1];
		w *= ((double *)(mask->buf + i))[3];
		y = ((double *)(colour->buf + i))[3];
		y = (1 - y) * w + y * (1 - w);
		((double *)(colour->buf + i))[3] = y;
	}
}

static void
process_xyza_i(struct stream *colour, struct stream *mask, size_t n)
{
	size_t i;
	double w, y;
	for (i = 0; i < n; i += colour->pixel_size) {
		w = 1 - ((double *)(mask->buf + i))[1];
		w *= ((double *)(mask->buf + i))[3];
		y = ((double *)(colour->buf + i))[3];
		y = (1 - y) * w + y * (1 - w);
		((double *)(colour->buf + i))[3] = y;
	}
}

int
main(int argc, char *argv[])
{
	int invert = 0;
	struct stream colour;
	struct stream mask;
	ssize_t r;
	size_t i, n;
	void (*process)(struct stream *colour, struct stream *mask, size_t n);

	ARGBEGIN {
	case 'i':
		invert = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 1)
		usage();

	colour.file = "<stdin>";
	colour.fd = STDIN_FILENO;
	einit_stream(&colour);

	mask.file = argv[0];
	mask.fd = open(mask.file, O_RDONLY);
	if (mask.fd < 0)
		eprintf("open %s:", mask.file);
	einit_stream(&mask);

	if (colour.width != mask.width || colour.height != mask.height)
		eprintf("videos do not have the same geometry\n");
	if (colour.pixel_size != mask.pixel_size)
		eprintf("videos use incompatible pixel formats\n");

	if (!strcmp(colour.pixfmt, "xyza"))
		process = invert ? process_xyza_i : process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	for (;;) {
		if (colour.ptr < sizeof(colour.buf) && !eread_stream(&colour, SIZE_MAX)) {
			close(colour.fd);
			colour.fd = -1;
			break;
		}
		if (mask.ptr < sizeof(mask.buf) && !eread_stream(&mask, SIZE_MAX)) {
			close(mask.fd);
			mask.fd = -1;
			break;
		}

		n = colour.ptr < mask.ptr ? colour.ptr : mask.ptr;
		n -= n % colour.pixel_size;
		colour.ptr -= n;
		mask.ptr -= n;

		process(&colour, &mask, n);

		for (i = 0; i < n; i += (size_t)r) {
			r = write(STDOUT_FILENO, colour.buf + i, n - i);
			if (r < 0)
				eprintf("write <stdout>:");
		}

		if ((n & 3) || colour.ptr != mask.ptr) {
			memmove(colour.buf, colour.buf + n, colour.ptr);
			memmove(mask.buf,   mask.buf  + n,  mask.ptr);
		}
	}

	if (mask.fd >= 0)
		close(mask.fd);

	for (i = 0; i < colour.ptr; i += (size_t)r) {
		r = write(STDOUT_FILENO, colour.buf + i, colour.ptr - i);
		if (r < 0)
			eprintf("write <stdout>:");
	}

	if (colour.fd >= 0) {
		for (;;) {
			colour.ptr = 0;
			if (!eread_stream(&colour, SIZE_MAX)) {
				close(colour.fd);
				colour.fd = -1;
				break;
			}

			for (i = 0; i < colour.ptr; i += (size_t)r) {
				r = write(STDOUT_FILENO, colour.buf + i, colour.ptr - i);
				if (r < 0)
					eprintf("write <stdout>:");
			}
		}
	}

	return 0;
}
