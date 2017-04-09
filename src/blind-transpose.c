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
	char *buf, *image;
	size_t n, imgw, srcw, srch, ps, x, y, i, b, dx;

	UNOFLAGS(argc);

	eopen_stream(&stream, NULL);
	imgw = srch = stream.height;
	stream.height = srcw = stream.width;
	stream.width = imgw;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, stream.file);
	n = stream.width * stream.height * (ps = stream.pixel_size);
	buf   = emalloc(n);
	image = emalloc(n); /* TODO optimise to a frame row */

	srch *= ps;
	srcw *= dx = imgw * ps;
	imgw *= ps;
	while (eread_frame(&stream, buf, n)) {
		for (b = y = 0; y < srch; y += ps)
			for (x = 0; x < srcw; x += dx)
				for (i = 0; i < ps; i++, b++)
					image[y + x + i] = buf[b];
		ewriteall(STDOUT_FILENO, image, n, "<stdout>");
	}

	free(buf);
	free(image);
	return 0;
}
