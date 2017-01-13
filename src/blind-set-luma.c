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
	void (*process)(struct stream *colour, struct stream *luma, size_t n);

	ENOFLAGS(argc != 1);

	colour.file = "<stdin>";
	colour.fd = STDIN_FILENO;
	einit_stream(&colour);

	luma.file = argv[0];
	luma.fd = eopen(luma.file, O_RDONLY);
	einit_stream(&luma);

	if (!strcmp(colour.pixfmt, "xyza"))
		process = process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	process_two_streams(&colour, &luma, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
