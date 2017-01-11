/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

USAGE("[-t] width height left top")

int
main(int argc, char *argv[])
{
	struct stream stream;
	char *buf, *image, *p;
	size_t width = 0, height = 0, left = 0, top = 0;
	size_t off, yoff = 0, x, y, irown, orown, ptr, n, m;
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

	width  = etozu_arg("the width",         argv[0], 1, SIZE_MAX);
	height = etozu_arg("the height",        argv[1], 1, SIZE_MAX);
	left   = etozu_arg("the left position", argv[2], 0, SIZE_MAX);
	top    = etozu_arg("the top position",  argv[3], 0, SIZE_MAX);

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
	efflush(stdout, "<stdout>");

	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, stream.file);
	n = stream.height * (irown = stream.width * stream.pixel_size);
	buf = emalloc(n);
	orown = width * stream.pixel_size;
	m = tile ? n : height * orown;
	image = emalloc(m);

	left *= stream.pixel_size;
	if (!tile) {
		off = top * irown;
	} else {
		off  = (orown  - (left % orown))  % orown;
		yoff = (height - (top  % height)) % height;
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

		ewriteall(STDOUT_FILENO, image, m, "<stdout>");
	}

	return 0;
}
