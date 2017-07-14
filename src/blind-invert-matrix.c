/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("")

static int equal = 0;

#define SUB_ROWS()\
	do {\
		p2 = matrix + r2 * cn;\
		t = p2[r1][0];\
		for (c = 0; c < cn; c++)\
			p1[c][0] -= p2[c][0] * t;\
	} while (0)

#define PROCESS(TYPE)\
	do {\
		typedef TYPE pixel_t[4];\
		size_t rn = stream->height, r1, r2, c;\
		size_t cn = stream->width > rn ? stream->width : 2 * rn;\
		pixel_t *matrix = buf, *p1, *p2 = NULL;\
		TYPE t;\
		\
		for (r1 = 0; r1 < rn; r1++) {\
			p1 = matrix + r1 * cn;\
 			if (!p1[r1][0]) {\
				for (r2 = r1 + 1; r2 < rn; r2++) {\
					p2 = matrix + r2 * cn;\
					if (p2[r1][0])\
						break;\
				}\
				if (r2 == rn)\
					eprintf("matrix is not invertable\n");\
				for (c = 0; c < cn; c++)\
					t = p1[c][0], p1[c][0] = p2[c][0], p2[c][0] = t;\
			}\
			t = p1[r1][0];\
			for (c = 0; c < cn; c++)\
				p1[c][0] /= t;\
			for (r2 = r1 + 1; r2 < rn; r2++)\
				SUB_ROWS();\
		}\
		\
		for (r1 = rn; r1--;) {\
			p1 = matrix + r1 * cn;\
			for (r2 = r1; r2--;)\
				SUB_ROWS();\
		}\
	} while (0)

static void process_lf(struct stream *stream, void *buf) { PROCESS(double); }
static void process_f (struct stream *stream, void *buf) { PROCESS(float); }

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t width, x, y, row_size, chan_size;
	char *buf, *one = alloca(4 * sizeof(double)), *p;
	void (*process)(struct stream *stream, void *buf);

	ARGBEGIN {
	case 'e':
		equal = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc)
		usage();

	eopen_stream(&stream, NULL);
	echeck_dimensions(&stream, WIDTH | HEIGHT, NULL);
	width = stream.width;
	if (stream.width < stream.height)
		eprintf("<stdin>: the video must be at least as wide as it is tall\n");
	else if (stream.width > stream.height)
		stream.width -= stream.height;
	fprint_stream_head(stdout, &stream);
	stream.width = width;
	efflush(stdout, "<stdout>");

	if (!strcmp(stream.pixfmt, "xyza")) {
		*(double *)one = 1;
		process = process_lf;
	} else if (!strcmp(stream.pixfmt, "xyza f")) {
		*(float *)one = 1;
		process = process_f;
	} else {
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);
	}

	chan_size = stream.pixel_size / 4;
	memcpy(one + 1 * chan_size, one, chan_size);
	memcpy(one + 2 * chan_size, one, chan_size);
	memcpy(one + 3 * chan_size, one, chan_size);

	width = stream.width > stream.height ? stream.width : 2 * stream.height;
	buf = emalloc2(width, stream.col_size);
	row_size = width * stream.pixel_size;

	while (eread_frame(&stream, buf)) {
		if (stream.width == stream.height) {
			for (y = stream.height; y--;) {
				memmove(buf + y * row_size, buf + y * stream.row_size, stream.row_size);
				memset(buf + y * row_size + stream.row_size, 0, stream.row_size);
				memcpy(buf + y * row_size + y * stream.pixel_size, one, stream.pixel_size);
			}
		}
		if (equal) {
			process(&stream, buf + 1 * chan_size);
			for (y = 0; y < stream.height; y++) {
				for (x = 0; x < stream.width; x++) {
					p = buf + y * row_size + x * stream.pixel_size;
					memcpy(p + chan_size, p, chan_size);
					memcpy(p + 2 * chan_size, p, 2 * chan_size);
				}
			}
		} else {
			process(&stream, buf + 0 * chan_size);
			process(&stream, buf + 1 * chan_size);
			process(&stream, buf + 2 * chan_size);
			process(&stream, buf + 3 * chan_size);
		}
		for (y = 0; y < stream.height; y++)
			ewriteall(STDOUT_FILENO, buf + y * row_size + stream.row_size, stream.row_size, "<stdout>");
	}

	free(buf);
	return 0;
}
