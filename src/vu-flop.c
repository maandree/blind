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
	char *buf, t1, t2;
	size_t ptr, n, i, j, k, wm;
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
		eprintf("<stdin>:");

	if (stream.width > SIZE_MAX / stream.pixel_size)
		eprintf("<stdin>: video is too wide\n");
	n = stream.width * stream.pixel_size;
	if (!(buf = malloc(n)))
		eprint("malloc:");

	wm = stream.width - 1;
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
		for (i = 0; i < stream.pixel_size; i++) {
			for (j = 0; j < stream.width >> 1; j++) {
				k = wm - j;
				t1 = buf[j * stream.pixel_size + i];
				t2 = buf[k * stream.pixel_size + i];
				buf[j * stream.pixel_size + i] = t2;
				buf[k * stream.pixel_size + i] = t1;
			}
		}
		for (ptr = 0; ptr < n; ptr += (size_t)r) {
			r = write(STDOUT_FILENO, buf + ptr, n - ptr);
			if (r < 0)
				eprintf("write <stdout>");
		}
	}

	return 0;
}
