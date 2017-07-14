/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("[-c]")

static int per_channel = 0;

#define PROCESS(TYPE)\
	do {\
		typedef TYPE pixel_t[4];\
		pixel_t matrix[9];\
		pixel_t buf;\
		int i;\
		\
		for (i = 0; i < 4; i++) {\
			matrix[0][i] = 1, matrix[1][i] = 0, matrix[2][i] = 0;\
			matrix[3][i] = 0, matrix[4][i] = 1, matrix[5][i] = 0;\
			matrix[6][i] = 0, matrix[7][i] = 0, matrix[8][i] = 1;\
		}\
		\
		while (eread_frame(stream, buf)) {\
			if (per_channel) {\
				for (i = 0; i < 4; i++) {\
					matrix[3][i] = matrix[1][i] = buf[i];\
					matrix[4][i] = matrix[0][i] = 1 - buf[i];\
				}\
			} else {\
				for (i = 0; i < 4; i++) {\
					matrix[3][i] = matrix[1][i] = buf[1] * buf[3];\
					matrix[4][i] = matrix[0][i] = 1 - matrix[3][i];\
				}\
			}\
			ewriteall(STDOUT_FILENO, matrix, sizeof(matrix), "<stdout>");\
		}\
	} while (0)

static void process_lf(struct stream *stream) { PROCESS(double); }
static void process_f(struct stream *stream) { PROCESS(float); }

int
main(int argc, char *argv[])
{
	struct stream stream;
	void (*process)(struct stream *stream);

	ARGBEGIN {
	case 'c':
		per_channel = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc)
		usage();

	eopen_stream(&stream, NULL);

	if (stream.width != 1 || stream.height != 1)
		eprintf("<stdin>: each frame must contain exactly 1 pixels\n");

	stream.width  = 3;
	stream.height = 3;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_lf;
	else if (!strcmp(stream.pixfmt, "xyza f"))
		process = process_f;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	process(&stream);
	return 0;
}
