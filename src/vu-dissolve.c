/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <string.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s [-r]\n", argv0);
}

static void
process_xyza(struct stream *stream, size_t n, size_t f, size_t fm)
{
	size_t i;
	double a;
	for (i = 0; i < n; i += stream->pixel_size) {
		a = ((double *)(stream->buf + i))[3];
		a = a * (double)f / fm;
		((double *)(stream->buf + i))[3] = a;
	}
}

static void
process_xyza_r(struct stream *stream, size_t n, size_t f, size_t fm)
{
	size_t i;
	double a;
	for (i = 0; i < n; i += stream->pixel_size) {
		a = ((double *)(stream->buf + i))[3];
		a = a * (double)(fm - f) / fm;
		((double *)(stream->buf + i))[3] = a;
	}
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	int reverse = 0;
	size_t f, h, w;
	size_t n, i, fm;
	ssize_t r;
	void (*process)(struct stream *stream, size_t n, size_t f, size_t fm) = NULL;

	ARGBEGIN {
	case 'r':
		reverse = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc)
		usage();

	stream.fd = STDIN_FILENO;
	stream.file = "<stdin>";
	einit_stream(&stream);
	fprint_stream_head(stdout, &stream);
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");

	if (!strcmp(stream.pixfmt, "xyza"))
		process = reverse ? process_xyza_r : process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	fm = stream.frames - 1;
	for (f = 0; f < stream.frames; f++) {
		for (h = stream.height; h--;) {
			for (w = stream.width * stream.pixel_size; w; w -= n) {
				if (!eread_stream(&stream, w))
					eprintf("<stdin>: file is shorter than expected\n");
				n = stream.ptr - (stream.ptr % stream.pixel_size);
				process(&stream, n, f, fm);
				for (i = 0; i < n; i += (size_t)r) {
					r = write(STDOUT_FILENO, stream.buf + i, n - i);
					if (r < 0)
						eprintf("write <stdout>:");
				}
				memmove(stream.buf, stream.buf + n, stream.ptr -= n);
			}
		}
	}

	return 0;
}
