/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <string.h>
#include <unistd.h>

USAGE("[-r]")

static size_t fm;
static double fmd;

static void
process_xyza(struct stream *stream, size_t n, size_t f)
{
	size_t i;
	double a;
	for (i = 0; i < n; i += stream->pixel_size) {
		a = ((double *)(stream->buf + i))[3];
		a = a * (double)(fm - f) / fmd;
		((double *)(stream->buf + i))[3] = a;
	}
}

static void
process_xyza_r(struct stream *stream, size_t n, size_t f)
{
	size_t i;
	double a;
	for (i = 0; i < n; i += stream->pixel_size) {
		a = ((double *)(stream->buf + i))[3];
		a = a * (double)f / fmd;
		((double *)(stream->buf + i))[3] = a;
	}
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	int reverse = 0;
	void (*process)(struct stream *stream, size_t n, size_t f) = NULL;

	ARGBEGIN {
	case 'r':
		reverse = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc)
		usage();

	eopen_stream(&stream, NULL);

	if (!strcmp(stream.pixfmt, "xyza"))
		process = reverse ? process_xyza_r : process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	fmd = fm = stream.frames - 1;
	process_each_frame_segmented(&stream, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
