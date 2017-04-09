/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <alloca.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

USAGE("[-L] (file (end-point | 'end')) ...")

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t *ends, i, parts, ptr, end, frame_size, n;
	char *to_end;
	FILE *fp;
	int fd, unknown_length = 0;

	ARGBEGIN {
	case 'L':
		unknown_length = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc % 2 || !argc)
		usage();

	eopen_stream(&stream, NULL);
	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, stream.file);
	frame_size = stream.width * stream.height * stream.pixel_size;
	if (stream.frames > (size_t)SSIZE_MAX / frame_size)
		eprintf("%s: video is too large\n", stream.file);

	parts = (size_t)argc / 2;
	ends = alloca(parts * sizeof(*ends));
	to_end = alloca(parts);

	for (i = 0; i < parts; i++) {
		to_end[i] = 0;
		if (!strcmp(argv[i * 2 + 1], "end")) {
			ends[i] = unknown_length ? SIZE_MAX : stream.frames;
			to_end[i] = 1;
		} else if (tozu(argv[i * 2 + 1], 0, SIZE_MAX, ends + i)) {
			eprintf("the end point must be an integer in [0, %zu]\n", SIZE_MAX);
		}
		if (i && ends[i] <= ends[i - 1])
			eprintf("the end points must be in strictly ascending order\n");
		if (!unknown_length && ends[i] > stream.frames)
			eprintf("frame %zu is beyond the end of the video\n", ends[i]);
	}

	ptr = 0;
	for (i = 0; i < parts; i++) {
		fd = eopen(argv[i * 2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
		fp = fdopen(fd, "wb");
		if (!fp)
			eprintf("fdopen %s:", argv[i * 2]);

		stream.frames = ends[i] - (i ? ends[i - 1] : 0);
		fprint_stream_head(fp, &stream);
		efflush(fp, argv[i * 2]);

		for (end = to_end[i] ? SIZE_MAX : ends[i] * frame_size; ptr < end; ptr += n) {
			n = end - ptr;
			if (stream.ptr) {
				n = MIN(stream.ptr, n);
				ewriteall(fd, stream.buf, n, argv[i * 2]);
				memmove(stream.buf, stream.buf + n, stream.ptr -= n);
			} else if ((n = eread_stream(&stream, n))) {
				ewriteall(fd, stream.buf, n, argv[i * 2]);
				stream.ptr = 0;
			} else if (ptr % frame_size) {
				eprintf("%s: incomplete frame\n", stream.file);
			} else if (!unknown_length || !to_end[i]) {
				eprintf("%s: file is shorter than expected\n", stream.file);
			} else {
				break;
			}
		}

		if (fclose(fp))
			eprintf("%s:", argv[i * 2]);
		close(fd);
	}

	return 0;
}
