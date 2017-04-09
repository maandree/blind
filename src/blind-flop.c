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

	UNOFLAGS(argc);

	eopen_stream(&stream, NULL);
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	echeck_frame_size(stream.width, 1, stream.pixel_size, 0, stream.file);
	n = stream.width * stream.pixel_size;
	buf   = emalloc(n);
	image = emalloc(n);

	m = n - stream.pixel_size;
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
