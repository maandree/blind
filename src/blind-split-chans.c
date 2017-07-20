/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("X-file Y-file Z-file [alpha-file]")

int
main(int argc, char *argv[])
{
	struct stream stream;
	char xbuf[BUFSIZ], ybuf[BUFSIZ], zbuf[BUFSIZ], abuf[BUFSIZ];
	int xfd, yfd, zfd, afd = -1;
	size_t i, n, ptr;

	UNOFLAGS(argc != 3 && argc != 4);

	eopen_stream(&stream, NULL);

	xfd = eopen(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);
	yfd = eopen(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
	zfd = eopen(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (argc == 4)
		afd = eopen(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0666);

	if (DPRINTF_HEAD(xfd, stream.frames, stream.width, stream.height, stream.pixfmt) < 0)
		eprintf("dprintf %s:", argv[0]);
	if (DPRINTF_HEAD(yfd, stream.frames, stream.width, stream.height, stream.pixfmt) < 0)
		eprintf("dprintf %s:", argv[1]);
	if (DPRINTF_HEAD(zfd, stream.frames, stream.width, stream.height, stream.pixfmt) < 0)
		eprintf("dprintf %s:", argv[2]);
	if (afd >= 0 && DPRINTF_HEAD(afd, stream.frames, stream.width, stream.height, stream.pixfmt) < 0)
		eprintf("dprintf %s:", argv[3]);

	n = (stream.n_chan - (afd < 0)) * stream.chan_size;
	do {
		for (ptr = 0; ptr + stream.pixel_size <= stream.ptr; ptr += stream.pixel_size) {
			for (i = 0; i < n; i += stream.chan_size) {
				memcpy(xbuf + ptr + i, stream.buf + ptr + 0 * stream.chan_size, stream.chan_size);
				memcpy(ybuf + ptr + i, stream.buf + ptr + 1 * stream.chan_size, stream.chan_size);
				memcpy(zbuf + ptr + i, stream.buf + ptr + 2 * stream.chan_size, stream.chan_size);
				if (afd >= 0)
					memcpy(abuf + ptr + i, stream.buf + ptr + 3 * stream.chan_size, stream.chan_size);
			}
			if (afd < 0) {
				memcpy(xbuf + ptr + n, stream.buf + ptr + 3 * stream.chan_size, stream.chan_size);
				memcpy(ybuf + ptr + n, stream.buf + ptr + 3 * stream.chan_size, stream.chan_size);
				memcpy(zbuf + ptr + n, stream.buf + ptr + 3 * stream.chan_size, stream.chan_size);
			}
		}
		ewriteall(xfd, xbuf, ptr, argv[0]);
		ewriteall(yfd, ybuf, ptr, argv[1]);
		ewriteall(zfd, zbuf, ptr, argv[2]);
		if (afd >= 0)
			ewriteall(afd, abuf, ptr, argv[3]);
		memmove(stream.buf, stream.buf + ptr, stream.ptr -= ptr);
	} while (eread_stream(&stream, SIZE_MAX));
	if (stream.ptr)
		eprintf("%s: incomplete frame\n", stream.file);

	close(xfd);
	close(yfd);
	close(zfd);
	if (afd >= 0)
		close(afd);
	return 0;
}
