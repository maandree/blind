/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

USAGE("[-b] bottom-stream ... top-stream")

static void
process_xyza(struct stream *streams, size_t n_streams, size_t n)
{
	double x1, y1, z1, a1;
	double x2, y2, z2, a2;
	size_t i, j;
	for (i = 0; i < n; i += streams->pixel_size) {
		x1 = ((double *)(streams[0].buf + i))[0];
		y1 = ((double *)(streams[0].buf + i))[1];
		z1 = ((double *)(streams[0].buf + i))[2];
		a1 = ((double *)(streams[0].buf + i))[3];
		for (j = 1; j < n_streams; j++) {
			x2 = ((double *)(streams[j].buf + i))[0];
			y2 = ((double *)(streams[j].buf + i))[1];
			z2 = ((double *)(streams[j].buf + i))[2];
			a2 = ((double *)(streams[j].buf + i))[3];
			x1 = x1 * a1 * (1 - a2) + x2 * a2;
			y1 = y1 * a1 * (1 - a2) + y2 * a2;
			z1 = z1 * a1 * (1 - a2) + z2 * a2;
			a1 = a1 * (1 - a2) + a2;
		}
		((double *)(streams[0].buf + i))[0] = x1;
		((double *)(streams[0].buf + i))[1] = y1;
		((double *)(streams[0].buf + i))[2] = z1;
		((double *)(streams[0].buf + i))[3] = a1;
	}
}

static void
process_xyza_b(struct stream *streams, size_t n_streams, size_t n)
{
	double x1, y1, z1, a1;
	double x2, y2, z2, a2;
	size_t i, j;
	for (i = 0; i < n; i += streams->pixel_size) {
		x1 = ((double *)(streams[0].buf + i))[0];
		y1 = ((double *)(streams[0].buf + i))[1];
		z1 = ((double *)(streams[0].buf + i))[2];
		a1 = ((double *)(streams[0].buf + i))[3];
		for (j = 1; j < n_streams;) {
			x2 = ((double *)(streams[j].buf + i))[0];
			y2 = ((double *)(streams[j].buf + i))[1];
			z2 = ((double *)(streams[j].buf + i))[2];
			a2 = ((double *)(streams[j].buf + i))[3];
			a2 /= ++j;
			x1 = x1 * a1 * (1 - a2) + x2 * a2;
			y1 = y1 * a1 * (1 - a2) + y2 * a2;
			z1 = z1 * a1 * (1 - a2) + z2 * a2;
			a1 = a1 * (1 - a2) + a2;
		}
		((double *)(streams[0].buf + i))[0] = x1;
		((double *)(streams[0].buf + i))[1] = y1;
		((double *)(streams[0].buf + i))[2] = z1;
		((double *)(streams[0].buf + i))[3] = a1;
	}
}

int
main(int argc, char *argv[])
{
	struct stream *streams;
	size_t n_streams;
	int blend = 0;
	size_t i, j, n;
	size_t closed;
	void (*process)(struct stream *streams, size_t n_streams, size_t n) = NULL;

	ARGBEGIN {
	case 'b':
		blend = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc < 2)
		usage();

	n_streams = (size_t)argc;
	streams = ecalloc(n_streams, sizeof(*streams));

	for (i = 0; i < n_streams; i++) {
		streams[i].file = argv[i];
		streams[i].fd = eopen(streams[i].file, O_RDONLY);
		if (i)
			echeck_compat(streams + i, streams);
	}

	if (!strcmp(streams->pixfmt, "xyza"))
		process = blend ? process_xyza_b : process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", streams->pixfmt);

	while (n_streams) {
		n = SIZE_MAX;
		for (i = 0; i < n_streams; i++) {
			if (streams[i].ptr < sizeof(streams->buf) && !eread_stream(streams + i, SIZE_MAX)) {
				close(streams[i].fd);
				streams[i].fd = -1;
			}
			if (streams[i].ptr && streams[i].ptr < n)
				n = streams[i].ptr;
		}
		n -= n % streams->pixel_size;

		process(streams, n_streams, n);

		ewriteall(STDOUT_FILENO, streams->buf, n, "<stdout>");

		closed = SIZE_MAX;
		for (i = 0; i < n_streams; i++) {
			memmove(streams[i].buf, streams[i].buf + n, streams[i].ptr -= n);
			if (streams[i].ptr < streams->pixel_size && streams[i].fd < 0 && closed == SIZE_MAX)
				closed = i;
		}
		if (closed != SIZE_MAX) {
			for (i = (j = closed) + 1; i < n_streams; i++)
				if (streams[i].ptr < streams->pixel_size && streams[i].fd < 0)
					streams[j++] = streams[i];
			n_streams = j;
		}
	}

	free(streams);
	return 0;
}
