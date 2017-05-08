/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

USAGE("")

static size_t srcw, srch, srcwps, srchps, ps;

#define PROCESS(TYPE)\
	do {\
		size_t x, i, n = ps / sizeof(TYPE);\
		char *src, *img;\
		for (x = 0; x < srchps; x += ps) {\
			img = row + x;\
			src = col + x * srcw;\
			for (i = 0; i < n; i++)\
				((TYPE *)img)[i] = ((TYPE *)src)[i];\
		}\
	} while (0)

static void process_double(char *row, char *col) {PROCESS(double);}
static void process_float (char *row, char *col) {PROCESS(float);}
static void process_char  (char *row, char *col) {PROCESS(char);}

int
main(int argc, char *argv[])
{
	struct stream stream;
	char *buf, *image;
	size_t n, y;
	void (*process)(char *col, char *row);

	UNOFLAGS(argc);

	eopen_stream(&stream, NULL);
	srch = stream.height;
	stream.height = srcw = stream.width;
	stream.width = srch;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, stream.file);
	n = stream.height * stream.width * (ps = stream.pixel_size);
	srchps = srch * ps;
	srcwps = srcw * ps;
	buf   = emalloc(n);
	image = emalloc(srchps);

	process = !(ps % sizeof(double)) ? process_double :
		  !(ps % sizeof(float))  ? process_float  : process_char;

	while (eread_frame(&stream, buf, n)) {
		for (y = 0; y < srcwps; y += ps) {
			process(image, buf + y);
			ewriteall(STDOUT_FILENO, image, srchps, "<stdout>");
		}
	}

	free(buf);
	free(image);
	return 0;
}
