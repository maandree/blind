/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

USAGE("")

int
main(int argc, char *argv[])
{
	struct stream stream;
	char *buf, *image;
	size_t i, j, n, m;

	ENOFLAGS(argc);

	stream.file = "<stdin>";
	stream.fd = STDIN_FILENO;
	einit_stream(&stream);
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	if (stream.width > SIZE_MAX / stream.pixel_size)
		eprintf("<stdin>: video frame is too wide\n");
	n = stream.width * stream.pixel_size;
	buf   = emalloc(n);
	image = emalloc(n);

	m = n - stream.pixel_size;
	memcpy(buf, stream.buf, stream.ptr);
	while (eread_row(&stream, buf, n)) {
		for (i = 0; i < stream.pixel_size; i++)
			for (j = 0; j < n; j += stream.pixel_size)
				image[m - j + i] = buf[i + j];
		ewriteall(STDOUT_FILENO, image, n, "<stdout>");
	}

	free(buf);
	free(image);
	return 0;
}
