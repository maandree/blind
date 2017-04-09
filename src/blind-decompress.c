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
	char *buf;
	size_t n, m, fptr, sptr, same = 0, diff = 0;

	UNOFLAGS(argc);

	eopen_stream(&stream, NULL);
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, "<stdin>");
	n = stream.width * stream.height * stream.pixel_size;
	buf = ecalloc(1, n);

	fptr = 0;
	do {
		sptr = 0;
	again:
		while (same) {
			m = MIN(same, n - fptr);
			ewriteall(STDOUT_FILENO, buf + fptr, m, "<stdout>");
			fptr = (fptr + m) % n;
			same -= m;
		}

		while (diff && sptr < stream.ptr) {
			m = MIN(diff, n - fptr);
			m = MIN(m, stream.ptr - sptr);
			memcpy(buf + fptr, stream.buf + sptr, m);
			ewriteall(STDOUT_FILENO, buf + fptr, m, "<stdout>");
			fptr = (fptr + m) % n;
			diff -= m;
			sptr += m;
		}

		if (diff || sptr + 2 * sizeof(size_t) > stream.ptr) {
			memmove(stream.buf, stream.buf + sptr, stream.ptr -= sptr);
		} else {
			same = ((size_t *)(stream.buf + sptr))[0];
			diff = ((size_t *)(stream.buf + sptr))[1];
			sptr += 2 * sizeof(size_t);
			goto again;
		}
	} while (eread_stream(&stream, SIZE_MAX));

	free(buf);
	if (same || diff)
		eprintf("<stdin>: corrupt input\n");
	return 0;
}
