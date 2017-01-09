/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#define eprintf(...) enprintf(2, __VA_ARGS__)

static void
usage(void)
{
	eprintf("usage: %s width height pixel-format ...\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t ptr, n, w;
	ssize_t r;
	int i, anything = 0;
	char *p;

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	if (argc < 3)
		usage();

	stream.frames = 1;
	stream.fd = STDIN_FILENO;
	stream.file = "<stdin>";
	stream.pixfmt[0] = '\0';

	if (tozu(argv[0], 1, SIZE_MAX, &stream.width))
		eprintf("the width must be an integer in [1, %zu]\n", SIZE_MAX);
	if (tozu(argv[1], 1, SIZE_MAX, &stream.height))
		eprintf("the height must be an integer in [1, %zu]\n", SIZE_MAX);
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
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");

	while (stream.height) {
		stream.height--;
		for (w = stream.width * stream.pixel_size; w; w -= n) {
			stream.ptr = 0;
			n = eread_stream(&stream, w);
			if (n == 0)
				goto done;
			anything = 1;
			for (ptr = 0; ptr < stream.ptr; ptr += (size_t)r) {
				r = write(STDOUT_FILENO, stream.buf + ptr, stream.ptr - ptr);
				if (r < 0)
					eprintf("write <stdin>:");
			}
		}
	}
done:

	if (stream.height || w)
		eprintf("incomplete frame\n");

	return !anything;
}
