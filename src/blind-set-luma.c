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
	double a, y;
	for (i = 0; i < n; i += colour->pixel_size) {
		a = ((double *)(luma->buf + i))[1];
		a *= ((double *)(luma->buf + i))[3];
		y = ((double *)(colour->buf + i))[1];
		((double *)(colour->buf + i))[0] += y * a - y;
		((double *)(colour->buf + i))[1]  = y * a;
		((double *)(colour->buf + i))[2] += y * a - y;
		/*
		 * Note, this changes the luma only, not the saturation,
		 * so the result may look a bit weird. To change both
		 * you can use `blind-arithm mul`.
		 * 
		 * Explaination:
		 *   Y is the luma, but (X, Z) is not the chroma,
		 *   but in CIELAB, L* is the luma and (a*, *b) is
		 *   the chroma. Multiplying
		 *   
		 *      ⎛0 1   0⎞
		 *      ⎜1 −1  0⎟
		 *      ⎝0  1 −1⎠
		 *   
		 *   (X Y Z)' gives a colour model similar to
		 *   CIE L*a*b*: a model where each parameter is
		 *   a linear transformation of the corresponding
		 *   parameter in CIE L*a*b*. The inverse of that
		 *   matrix is
		 *   
		 *      ⎛1 1  0⎞
		 *      ⎜1 0  0⎟
		 *      ⎝0 0 −1⎠
		 *   
		 *   and
		 *   
		 *      ⎛1 1  0⎞⎛a 0 0⎞⎛0 1   0⎞   ⎛1 a−1 0⎞
		 *      ⎜1 0  0⎟⎜0 1 0⎟⎜1 −1  0⎟ = ⎜0  a  0⎟.
		 *      ⎝0 0 −1⎠⎝0 0 1⎠⎝0  1 −1⎠   ⎝0 a−1 1⎠
		 */
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

	fprint_stream_head(stdout, &colour);
	efflush(stdout, "<stdout>");
	process_two_streams(&colour, &luma, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
