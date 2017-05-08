/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

USAGE("")

static struct stream stream;
static char *buf, *image;
static size_t n, m, ps;

#define PROCESS(TYPE)\
	do {\
		size_t i, j, pst = ps / sizeof(TYPE);\
		size_t nt = n / sizeof(TYPE);\
		size_t mt = m / sizeof(TYPE);\
		for (i = 0; i < pst; i++)\
			for (j = 0; j < nt; j += pst)\
				((TYPE *)image)[mt - j + i] = ((TYPE *)buf)[i + j];\
	} while (0)

static void process_double(void) {PROCESS(double);}
static void process_float (void) {PROCESS(float);}
static void process_char  (void) {PROCESS(char);}

int
main(int argc, char *argv[])
{
	void (*process)(void);

	UNOFLAGS(argc);

	eopen_stream(&stream, NULL);
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	echeck_frame_size(stream.width, 1, stream.pixel_size, 0, stream.file);
	n = stream.width * (ps = stream.pixel_size);
	buf   = emalloc(n);
	image = emalloc(n);

	process = !(ps % sizeof(double)) ? process_double :
		  !(ps % sizeof(float))  ? process_float  : process_char;

	m = n - ps;
	while (eread_row(&stream, buf, n)) {
		process();
		ewriteall(STDOUT_FILENO, image, n, "<stdout>");
	}

	free(buf);
	free(image);
	return 0;
}
