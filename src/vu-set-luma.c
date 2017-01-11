/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("luma-stream")

static void
process_xyza(struct stream *colour, struct stream *luma, size_t n)
{
	size_t i;
	double a;
	for (i = 0; i < n; i += colour->pixel_size) {
		a = ((double *)(luma->buf + i))[1];
		a *= ((double *)(luma->buf + i))[3];
		((double *)(colour->buf + i))[1] *= a;
	}
}

int
main(int argc, char *argv[])
{
	struct stream colour, luma;
	size_t n;
	void (*process)(struct stream *colour, struct stream *luma, size_t n);

	ENOFLAGS(argc != 1);

	colour.file = "<stdin>";
	colour.fd = STDIN_FILENO;
	einit_stream(&colour);

	luma.file = argv[0];
	luma.fd = eopen(luma.file, O_RDONLY);
	einit_stream(&luma);

	echeck_compat(&colour, &luma);

	if (!strcmp(colour.pixfmt, "xyza"))
		process = process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	for (;;) {
		if (colour.ptr < sizeof(colour.buf) && !eread_stream(&colour, SIZE_MAX)) {
			close(colour.fd);
			colour.fd = -1;
			break;
		}
		if (luma.ptr < sizeof(luma.buf) && !eread_stream(&luma, SIZE_MAX)) {
			close(luma.fd);
			luma.fd = -1;
			break;
		}

		n = colour.ptr < luma.ptr ? colour.ptr : luma.ptr;
		n -= n % colour.pixel_size;
		colour.ptr -= n;
		luma.ptr -= n;

		process(&colour, &luma, n);

		ewriteall(STDOUT_FILENO, colour.buf, n, "<stdout>");
		if ((n & 3) || colour.ptr != luma.ptr) {
			memmove(colour.buf, colour.buf + n, colour.ptr);
			memmove(luma.buf,   luma.buf   + n, luma.ptr);
		}
	}

	if (luma.fd >= 0)
		close(luma.fd);

	ewriteall(STDOUT_FILENO, colour.buf, colour.ptr, "<stdout>");

	if (colour.fd >= 0) {
		for (;;) {
			colour.ptr = 0;
			if (!eread_stream(&colour, SIZE_MAX)) {
				close(colour.fd);
				colour.fd = -1;
				break;
			}
			ewriteall(STDOUT_FILENO, colour.buf, colour.ptr, "<stdout>");
		}
	}

	return 0;
}
