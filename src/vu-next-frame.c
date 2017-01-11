/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#undef eprintf
#define eprintf(...) enprintf(2, __VA_ARGS__)

USAGE("width height pixel-format ...")

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t n, w;
	int i, anything = 0;
	char *p;

	ENOFLAGS(argc < 3);

	stream.frames = 1;
	stream.fd = STDIN_FILENO;
	stream.file = "<stdin>";
	stream.pixfmt[0] = '\0';

	stream.width  = etozu_arg("the width",  argv[0], 1, SIZE_MAX);
	stream.height = etozu_arg("the height", argv[1], 1, SIZE_MAX);
	argv += 2, argc -= 2;

	n = (size_t)argc - 1;
	for (i = 0; i < argc; i++)
		n += strlen(argv[i]);
	if (n < sizeof(stream.pixfmt)) {
		p = stpcpy(stream.pixfmt, argv[0]);
		for (i = 1; i < argc; i++) {
			*p++ = ' ';
			p = stpcpy(p, argv[i]);
		}
	}

	eset_pixel_size(&stream);

	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	w = stream.width * stream.pixel_size;
	while (stream.height) {
		stream.height--;
		for (n = w; n; n -= stream.ptr) {
			stream.ptr = 0;
			if (!eread_stream(&stream, n))
				goto done;
			anything = 1;
			ewriteall(STDOUT_FILENO, stream.buf, stream.ptr, "<stdout>");
		}
	}
done:

	if (stream.height || n)
		eprintf("incomplete frame\n");

	return !anything;
}
