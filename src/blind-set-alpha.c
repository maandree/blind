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

	eopen_stream(&colour, NULL);
	eopen_stream(&alpha, argv[0]);

	if (!strcmp(colour.pixfmt, "xyza"))
		process = invert ? process_xyza_i : process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	fprint_stream_head(stdout, &colour);
	efflush(stdout, "<stdout>");
	process_two_streams(&colour, &alpha, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
