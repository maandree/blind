/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <string.h>
#include <unistd.h>

USAGE("[-r]")

static size_t fm;
static double fm_double;
static float fm_float;

#define PROCESS(TYPE, NREV)\
	do {\
		size_t i;\
		TYPE a;\
		for (i = 0; i < n; i += stream->pixel_size) {\
			a = ((TYPE *)(stream->buf + i))[3];\
			a = a * (TYPE)(NREV f) / fm_##TYPE;\
			((TYPE *)(stream->buf + i))[3] = a;\
		}\
	} while (0)

static void process_xyza   (struct stream *stream, size_t n, size_t f) {PROCESS(double, fm -);}
static void process_xyza_r (struct stream *stream, size_t n, size_t f) {PROCESS(double,);}
static void process_xyzaf  (struct stream *stream, size_t n, size_t f) {PROCESS(float, fm -);}
static void process_xyzaf_r(struct stream *stream, size_t n, size_t f) {PROCESS(float,);}

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
	else if (!strcmp(stream.pixfmt, "xyza f"))
		process = reverse ? process_xyzaf_r : process_xyzaf;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	fm_double = fm = stream.frames - 1;
	fm_float = (float)fm_double;
	process_each_frame_segmented(&stream, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
