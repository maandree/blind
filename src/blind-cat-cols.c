/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("(file columns) ...")

int
main(int argc, char *argv[])
{
	struct stream *streams;
	size_t parts, width = 0, *cols, i;

	UNOFLAGS(argc % 2 || !argc);

	parts   = (size_t)argc / 2;
	streams = emalloc2(parts, sizeof(*streams));
	cols    = alloca(parts * sizeof(*cols));

	for (i = 0; i < parts; i++) {
		eopen_stream(streams + i, argv[i * 2]);
		cols[i] = etozu_arg("columns", argv[i * 2 + 1], 1, SIZE_MAX);
		if (streams[i].width > SIZE_MAX - width)
			eprintf("output video is too tall\n");
		width += streams[i].width;
		if (i) {
			streams[i].width = streams->width;
			echeck_compat(streams, streams + i);
		}
	}

	streams->width = width;
	fprint_stream_head(stdout, streams);
	efflush(stdout, "<stdout>");

	for (i = 0; i < parts; i++, i = i == parts ? 0 : i)
		if (esend_pixels(streams + i, STDOUT_FILENO, cols[i], "<stdout>") != cols[i])
			break;
	for (i = 0; i < parts; i++)
		close(streams[i].fd);

	free(streams);
	return 0;
}
