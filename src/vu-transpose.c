/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

static void
transpose(const struct stream *stream, const char *restrict buf, char *restrict image)
{
	size_t x, y, i, b = 0, h, w;
	w = stream->width * (h = stream->height * stream->pixel_size);
	for (y = 0; y < h; y += stream->pixel_size)
		for (x = 0; x < w; x += h)
			for (i = 0; i < stream->pixel_size; i++)
				image[x + y + i] = buf[b++];
}

static void
usage(void)
{
	eprintf("usage: %s\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	char *buf, *image;
	size_t ptr, n;
	ssize_t r;

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	if (argc)
		usage();

	stream.file = "<stdin>";
	stream.fd = STDIN_FILENO;
	einit_stream(&stream);
	fprint_stream_head(stdout, &stream);
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");

	if (stream.width > SIZE_MAX / stream.pixel_size)
		eprintf("<stdin>: video frame is too large\n");
	n = stream.width * stream.pixel_size;
	if (n > SIZE_MAX / stream.height)
		eprintf("<stdin>: video frame is too large\n");
	n *= stream.height;
	if (!(buf = malloc(n)))
		eprintf("malloc:");
	if (!(image = malloc(n)))
		eprintf("malloc:");

	memcpy(buf, stream.buf, ptr = stream.ptr);
	for (;;) {
		for (; ptr < n; ptr += (size_t)r) {
			r = read(stream.fd, buf + ptr, n - ptr);
			if (r < 0) {
				eprintf("read %s:", stream.file);
			} else if (r == 0) {
				if (!ptr)
					break;
				eprintf("%s: incomplete frame", stream.file);
			}
		}
		if (!ptr)
			break;
		transpose(&stream, buf, image);
		for (ptr = 0; ptr < n; ptr += (size_t)r) {
			r = write(STDOUT_FILENO, image + ptr, n - ptr);
			if (r < 0)
				eprintf("write <stdout>");
		}
	}

	return 0;
}
