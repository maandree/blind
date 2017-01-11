/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: [-t] width height left top %s\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	char *buf, *image, *p;
	size_t width = 0, height = 0, left = 0, top = 0;
	size_t off, yoff, x, y, irown, orown, ptr, n, m;
	ssize_t r;
	int tile = 0;

	ARGBEGIN {
	case 't':
		tile = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 4)
		usage();

	if (tozu(argv[0], 1, SIZE_MAX, &width))
		eprintf("width be an integer in [1, %zu]\n", SIZE_MAX);
	if (tozu(argv[1], 1, SIZE_MAX, &height))
		eprintf("height be an integer in [1, %zu]\n", SIZE_MAX);
	if (tozu(argv[2], 0, SIZE_MAX, &left))
		eprintf("left position be an integer in [0, %zu]\n", SIZE_MAX);
	if (tozu(argv[3], 0, SIZE_MAX, &top))
		eprintf("top position be an integer in [0, %zu]\n", SIZE_MAX);

	stream.file = "<stdin>";
	stream.fd = STDIN_FILENO;
	einit_stream(&stream);
	if (left + width > stream.width || top + height > stream.height)
		eprintf("crop area extends beyond original image\n");
	if (tile) {
		fprint_stream_head(stdout, &stream);
	} else {
		x = stream.width,  stream.width  = width;
		y = stream.height, stream.height = height;
		fprint_stream_head(stdout, &stream);
		stream.width  = x;
		stream.height = y;
	}
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");

	if (stream.width > SIZE_MAX / stream.pixel_size)
		eprintf("<stdin>: video frame is too large\n");
	n = irown = stream.width * stream.pixel_size;
	if (n > SIZE_MAX / stream.height)
		eprintf("<stdin>: video frame is too large\n");
	n *= stream.height;
	if (!(buf = malloc(n)))
		eprintf("malloc:");
	orown = width * stream.pixel_size;
	m = tile ? n : height * orown;
	if (!(image = malloc(m)))
		eprintf("malloc:");

	left *= stream.pixel_size;
	if (tile) {
		off  = (orown  - (left % orown))  % orown;
		yoff = (height - (top  % height)) % height;
	} else {
		off = top * irown;
	}
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

		if (!tile) {
			for (y = 0; y < height; y++)
				memcpy(image + y * orown, buf + y * irown + off, orown);
		} else {
			for (ptr = y = 0; y < stream.height; y++) {
				p = buf + ((y + yoff) % height) * irown + left;
				for (x = 0; x < irown; x++, ptr++)
					image[ptr++] = p[(x + off) % orown];
			}
			
		}

		for (ptr = 0; ptr < m; ptr += (size_t)r) {
			r = write(STDOUT_FILENO, image + ptr, m - ptr);
			if (r < 0)
				eprintf("write <stdout>");
		}
	}

	return 0;
}
