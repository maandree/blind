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
	eprintf("usage: %s [-l left] [-r right] [-a above] [-b below] [-t]\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	char *buf, *image;
	size_t ptr, n, m, imgw, imgh, rown;
	size_t xoff, yoff, h, x, y;
	size_t left = 0, right = 0, top = 0, bottom = 0;
	ssize_t r;
	int tile = 0;

	ARGBEGIN {
	case 'l':
		if (tozu(EARGF(usage()), 0, SIZE_MAX, &left))
			eprintf("argument of -l must be an integer in [0, %zu]\n", SIZE_MAX);
		break;
	case 'r':
		if (tozu(EARGF(usage()), 0, SIZE_MAX, &right))
			eprintf("argument of -r must be an integer in [0, %zu]\n", SIZE_MAX);
		break;
	case 'a':
		if (tozu(EARGF(usage()), 0, SIZE_MAX, &top))
			eprintf("argument of -a must be an integer in [0, %zu]\n", SIZE_MAX);
		break;
	case 'b':
		if (tozu(EARGF(usage()), 0, SIZE_MAX, &bottom))
			eprintf("argument of -b must be an integer in [0, %zu]\n", SIZE_MAX);
		break;
	case 't':
		tile = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc)
		usage();

	stream.file = "<stdin>";
	stream.fd = STDIN_FILENO;
	einit_stream(&stream);

	if (stream.width > SIZE_MAX / stream.pixel_size)
		eprintf("<stdin>: video frame is too large\n");
	n = stream.width * stream.pixel_size;
	if (n > SIZE_MAX / stream.height)
		eprintf("<stdin>: video frame is too large\n");
	n *= stream.height;
	if (!(buf = malloc(n)))
		eprintf("malloc:");

	if (stream.width > SIZE_MAX - left)
		eprintf("<stdout>: output video is too wide\n");
	imgw = stream.width + left;
	if (imgw > SIZE_MAX - right)
		eprintf("<stdout>: output video is too wide\n");
	imgw += right;
	if (stream.height > SIZE_MAX - top)
		eprintf("<stdout>: output video is too tall\n");
	imgh = stream.height + top;
	if (imgh > SIZE_MAX - bottom)
		eprintf("<stdout>: output video is too tall\n");
	imgh += bottom;
	if (imgw > SIZE_MAX / stream.pixel_size)
		eprintf("<stdout>: output video frame is too large\n");
	m = imgw *= stream.pixel_size;
	if (m > SIZE_MAX / imgh)
		eprintf("<stdout>: output video frame is too large\n");
	m *= imgh;
	if (!(image = malloc(m)))
		eprintf("malloc:");

	if (!tile)
		memset(image, 0, m);

	stream.width += left + right;
	h = stream.height += top + bottom;
	fprint_stream_head(stdout, &stream);
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");
	stream.width -= left + right;
	stream.height -= top + bottom;

	left  *= stream.pixel_size;
	right *= stream.pixel_size;
	rown = stream.width * stream.pixel_size;

	xoff = (rown          - (left % rown))          % rown;
	yoff = (stream.height - (top  % stream.height)) % stream.height;

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
			for (y = 0; y < stream.height; y++)
				memcpy(image + left + (y + top) * imgw, buf + y * rown, rown);
		} else {
			for (y = 0; y < stream.height; y++)
				for (x = 0; x < imgw; x++)
					image[x + (y + top) * imgw] = buf[((x + xoff) % rown) + y * rown];
			for (y = 0; y < top; y++)
				memcpy(image + y * imgw, image + (((y + yoff) % stream.height) + top) * imgw, imgw);
			for (y = top + stream.height; y < h; y++)
				memcpy(image + y * imgw, image + (((y + yoff) % stream.height) + top) * imgw, imgw);
		}

		for (ptr = 0; ptr < m; ptr += (size_t)r) {
			r = write(STDOUT_FILENO, image + ptr, m - ptr);
			if (r < 0)
				eprintf("write <stdout>");
		}
	}

	return 0;
}
