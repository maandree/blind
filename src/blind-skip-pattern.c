/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

USAGE("(skipped-frames | +included-frames) ...")

static int
process_frame(struct stream *stream, int include, size_t rown)
{
	size_t h, n;
	int anything = 0;

	for (h = stream->height; h; h--) {
		for (n = rown; n; n -= stream->ptr) {
			stream->ptr = 0;
			if (!eread_stream(stream, n))
				goto done;
			anything = 1;
			if (include)
				ewriteall(STDOUT_FILENO, stream->buf, stream->ptr, "<stdout>");
		}
	}
done:

	if (anything && h)
		eprintf("%s: is shorted than expected\n", stream->file);

	return !anything;
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	int i, include;
	size_t f, n, rown, total = 0;
	char *includes;
	size_t *ns;

	UNOFLAGS(!argc);

	eopen_stream(&stream, NULL);

	includes = emalloc((size_t)argc);
	ns = ecalloc((size_t)argc, sizeof(*ns));

	for (i = 0; i < argc; i++) {
		include = argv[i][0] == '+';
		n = etozu_arg(include ? "included frame count" : "skipped frame count",
			      argv[i] + include, 0, SIZE_MAX);
		ns[i] = n;
		includes[i] = (char)include;
	}
	if (!total)
		eprintf("null pattern is not allowed");

	for (i = 0, total = 0, f = stream.frames;; i = (i + 1) % argc) {
		include = (int)includes[i];
		for (n = ns[i]; n-- && f--;)
			total += (size_t)include;
	}

	echeck_frame_size(stream.width, 1, stream.pixel_size, 0, stream.file);
	rown = stream.width * stream.pixel_size;
	stream.frames = total;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	for (i = 0;; i = (i + 1) % argc) {
		include = (int)includes[i];
		n = ns[i];
		while (n-- && process_frame(&stream, include, rown));
	}

	free(includes);
	free(ns);
	return 0;
}
