/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("[-i] alpha-stream")

static void
process_xyza(struct stream *colour, struct stream *alpha, size_t n)
{
	size_t i;
	double a;
	for (i = 0; i < n; i += colour->pixel_size) {
		a = ((double *)(alpha->buf + i))[1];
		a *= ((double *)(alpha->buf + i))[3];
		((double *)(colour->buf + i))[3] *= a;
	}
}

static void
process_xyza_i(struct stream *colour, struct stream *alpha, size_t n)
{
	size_t i;
	double a;
	for (i = 0; i < n; i += colour->pixel_size) {
		a = 1 - ((double *)(alpha->buf + i))[1];
		a *= ((double *)(alpha->buf + i))[3];
		((double *)(colour->buf + i))[3] *= a;
	}
}

int
main(int argc, char *argv[])
{
	int invert = 0;
	struct stream colour, alpha;
	size_t n;
	void (*process)(struct stream *colour, struct stream *alpha, size_t n) = NULL;

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

	alpha.file = argv[0];
	alpha.fd = eopen(alpha.file, O_RDONLY);
	einit_stream(&alpha);

	echeck_compat(&colour, &alpha);

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
		if (alpha.ptr < sizeof(alpha.buf) && !eread_stream(&alpha, SIZE_MAX)) {
			close(alpha.fd);
			alpha.fd = -1;
			break;
		}

		n = colour.ptr < alpha.ptr ? colour.ptr : alpha.ptr;
		n -= n % colour.pixel_size;
		colour.ptr -= n;
		alpha.ptr -= n;

		process(&colour, &alpha, n);

		ewriteall(STDOUT_FILENO, colour.buf, n, "<stdout>");
		if ((n & 3) || colour.ptr != alpha.ptr) {
			memmove(colour.buf, colour.buf + n, colour.ptr);
			memmove(alpha.buf,  alpha.buf  + n, alpha.ptr);
		}
	}

	if (alpha.fd >= 0)
		close(alpha.fd);

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
