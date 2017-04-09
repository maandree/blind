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
		 * Explaination of algorithm:
		 * 
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
		 * 
		 * Explanation of why changing only the luma looks weird:
		 * 
		 *   Consider when you are workings with colours,
		 *   when you want to change the brightness of a
		 *   colour, you multiply all parameters: red, green,
		 *   and blue, with the same value (this is however
		 *   only an approximation in most cases, since you
		 *   are usually usally working with colours that
		 *   have the sRGB transfer function applied to their
		 *   parameters). This action is the same in all
		 *   colour models and colour spaces that are a
		 *   linear transformation of the sRGB colour spaces
		 *   (sans transfer function); this is simply because
		 *   of the properties of linear transformations.
		 * 
		 *   The reason you change brightness this way can
		 *   be explained by how objects reflect colour.
		 *   Objects can only reject colours that are present
		 *   in the light source. A ideal white object will look
		 *   pure red if the light sources is ideal red, and a
		 *   a ideal blue object will pure black in the same
		 *   light source. An object can also not reflect
		 *   colours brighter than the source. When the brightness
		 *   of a light source is changed, the intensity of all
		 *   colours (by wavelength) it emits is multiplied by
		 *   one value. Therefore, when changing the brightness
		 *   it looks most natural when all primaries (red, green,
		 *   and blue) are multiplied by one value, or all
		 *   parameters of the used colour spaces is a linear
		 *   transformation of sRGB, such as CIE XYZ.
		 */
	}
}

int
main(int argc, char *argv[])
{
	struct stream colour, luma;
	void (*process)(struct stream *colour, struct stream *luma, size_t n);

	UNOFLAGS(argc != 1);

	eopen_stream(&colour, NULL);
	eopen_stream(&luma, argv[0]);

	if (!strcmp(colour.pixfmt, "xyza"))
		process = process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	fprint_stream_head(stdout, &colour);
	efflush(stdout, "<stdout>");
	process_two_streams(&colour, &luma, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
