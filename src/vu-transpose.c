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
	size_t n, imgw, imgh, x, y, i, b;

	ENOFLAGS(argc);

	stream.file = "<stdin>";
	stream.fd = STDIN_FILENO;
	einit_stream(&stream);
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, "<stdin>");
	n = stream.width * stream.height * stream.pixel_size;
	buf   = emalloc(n);
	image = emalloc(n);

	imgw = stream.width * (imgh = stream.height * stream.pixel_size);
	memcpy(buf, stream.buf, stream.ptr);
	while (eread_frame(&stream, buf, n)) {
		for (b = y = 0; y < imgh; y += stream.pixel_size)
			for (x = 0; x < imgw; x += imgh)
				for (i = 0; i < stream.pixel_size; i++)
					image[x + y + i] = buf[b++];
		ewriteall(STDOUT_FILENO, image, n, "<stdout>");
	}

	free(buf);
	free(image);
	return 0;
}
