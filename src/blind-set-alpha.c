/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <string.h>

USAGE("[-i] alpha-stream")

#define PROCESS(TYPE, INV)\
	do {\
		size_t i;\
		TYPE a;\
		for (i = 0; i < n; i += colour->pixel_size) {\
			a = INV ((TYPE *)(alpha->buf + i))[1];\
			a *= ((TYPE *)(alpha->buf + i))[3];\
			((TYPE *)(colour->buf + i))[3] *= a;\
		}\
	} while (0)

static void process_xyza   (struct stream *colour, struct stream *alpha, size_t n) {PROCESS(double,);}
static void process_xyza_i (struct stream *colour, struct stream *alpha, size_t n) {PROCESS(double, 1 -);}
static void process_xyzaf  (struct stream *colour, struct stream *alpha, size_t n) {PROCESS(float,);}
static void process_xyzaf_i(struct stream *colour, struct stream *alpha, size_t n) {PROCESS(float, 1 -);}

int
main(int argc, char *argv[])
{
	int invert = 0;
	struct stream colour, alpha;
	void (*process)(struct stream *colour, struct stream *alpha, size_t n) = NULL;

	ARGBEGIN {
	case 'i':
		invert = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 1)
		usage();

	eopen_stream(&colour, NULL);
	eopen_stream(&alpha, argv[0]);

	if (!strcmp(colour.pixfmt, "xyza"))
		process = invert ? process_xyza_i : process_xyza;
	else if (!strcmp(colour.pixfmt, "xyza f"))
		process = invert ? process_xyzaf_i : process_xyzaf;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	fprint_stream_head(stdout, &colour);
	efflush(stdout, "<stdout>");
	process_two_streams(&colour, &alpha, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
