/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("(count | 'inf') file")

static void
repeat_regular_file(char *file, size_t count, int inf)
{
	struct stream stream;
	char buf[BUFSIZ];
	size_t ptr;
	ssize_t r;

	eopen_stream(&stream, file);
	if (count > SIZE_MAX / stream.frames)
		eprintf("%s: video is too long\n", stream.file);
	stream.frames *= count;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	while (inf || count--) {
		fadvise_sequential(stream.fd, stream.headlen, 0);
		for (ptr = stream.headlen;; ptr += (size_t)r) {
			if (!(r = epread(stream.fd, buf, sizeof(buf), ptr, stream.file)))
				break;
			if (writeall(STDOUT_FILENO, buf, (size_t)r)) {
				if (!inf || errno != EPIPE)
					eprintf("write <stdout>:");
				return;
			}
		}
	}

	close(stream.fd);
}

static void
repeat_stdin(size_t count, int inf)
{
	struct stream stream;
	char *buf;
	size_t ptr, size;
	ssize_t r;

	eopen_stream(&stream, NULL);
	if (count > SIZE_MAX / stream.frames)
		eprintf("%s: video is too long\n", stream.file);
	stream.frames *= count;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	ptr = stream.ptr;
	size = MAX(ptr, BUFSIZ);
	buf = emalloc(size);
	memcpy(buf, stream.buf, ptr);

	for (;;) {
		if (ptr == size)
			buf = erealloc(buf, size <<= 1);
		if (!(r = eread(STDIN_FILENO, buf + ptr, size - ptr, "<stdout>")))
			break;
		ptr += (size_t)r;
	}

	while (inf || count--) {
		if (writeall(STDOUT_FILENO, buf, ptr)) {
			if (!inf || errno != EPIPE)
				eprintf("write <stdout>:");
			return;
		}
	}

	free(buf);
}

int
main(int argc, char *argv[])
{
	size_t count = 0;
	int inf = 0;

	UNOFLAGS(argc != 2);

	if (!strcmp(argv[0], "inf"))
		inf = 1;
	else
		count = etozu_arg("the count", argv[0], 0, SIZE_MAX);

	if (inf)
		einf_check_fd(STDOUT_FILENO, "<stdout>");

	if (!strcmp(argv[1], "-"))
		repeat_stdin(count, inf);
	else
		repeat_regular_file(argv[1], count, inf);

	return 0;
}
