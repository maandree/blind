/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

USAGE("")

int
main(int argc, char *argv[])
{
	struct stream stream;
	char *buf, *row, *pix, *image, *imag, *img;
	size_t n, srcw, srch, srcwps, srchps, ps, x, y, i;

	UNOFLAGS(argc);

	eopen_stream(&stream, NULL);
	srch = stream.height;
	stream.height = srcw = stream.width;
	stream.width = srch;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, stream.file);
	n = stream.height * stream.width * (ps = stream.pixel_size);
	buf   = emalloc(n);
	image = emalloc(n);

	srchps = srch * ps;
	srcwps = srcw * ps;
	while (eread_frame(&stream, buf, n)) {
		for (y = 0; y < srchps; y += ps) {
			imag = image + y;
			row  = buf + y * srcw;
			for (x = 0; x < srcwps; x += ps) {
				img = imag + x * srch;
				pix = row + x;
				for (i = 0; i < ps; i++)
					img[i] = pix[i];
			}
		}
		ewriteall(STDOUT_FILENO, image, n, "<stdout>");
	}

	free(buf);
	free(image);
	return 0;
}
