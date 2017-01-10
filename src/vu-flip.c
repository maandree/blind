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
	eprintf("usage: %s\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	char *buf, *row;
	size_t ptr, n, i, j, hm, row_size;
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
		eprintf("<stdin>: video is too wide\n");
	row_size = stream.width * stream.pixel_size;
	if (!(row = malloc(row_size)))
		eprintf("malloc:");
	if (row_size > SIZE_MAX / stream.height)
		eprintf("<stdin>: video frame is too large\n");
	n = row_size * stream.height;
	if (!(buf = malloc(n)))
		eprintf("malloc:");

	hm = stream.height - 1;
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
		for (i = 0; i < stream.height >> 1; i++) {
			j = hm - i;
			memcpy(row, buf + i * row_size, row_size);
			memcpy(buf + i * row_size, buf + j * row_size, row_size);
			memcpy(buf + j * row_size, row, row_size);
		}
		for (ptr = 0; ptr < n; ptr += (size_t)r) {
			r = write(STDOUT_FILENO, buf + ptr, n - ptr);
			if (r < 0)
				eprintf("write <stdout>");
		}
	}

	return 0;
}
