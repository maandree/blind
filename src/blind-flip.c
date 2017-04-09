/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <string.h>
#include <unistd.h>

USAGE("")

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t n, rown, ptr;
	char *buf;

	UNOFLAGS(argc);

	eopen_stream(&stream, NULL);
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, stream.file);
	n = stream.height * (rown = stream.width * stream.pixel_size);
	buf = emalloc(n);

	while (eread_frame(&stream, buf, n))
		for (ptr = n; ptr;)
			ewriteall(STDOUT_FILENO, buf + (ptr -= rown), rown, "<stdout>");

	free(buf);
	return 0;
}
