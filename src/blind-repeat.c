/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <string.h>

USAGE("(count | 'inf') file")

static size_t count = 0;
static int inf;
static struct stream stream;

static int
repeat_regular_file(void)
{
	while (inf || count--) {
		fadvise_sequential(stream.fd, stream.headlen, 0);
		elseek(stream.fd, stream.headlen, SEEK_SET, stream.file);
		if (esend_stream(&stream, STDOUT_FILENO, NULL))
			return -1;
	}
	return 0;
}

static int
repeat_stdin(void)
{
	size_t ptr = stream.ptr;
	size_t size = MAX(ptr, BUFSIZ);
	char *buf = memcpy(emalloc(size), stream.buf, ptr);
	egetfile(STDIN_FILENO, &buf, &ptr, &size, "<stdout>");
	while (inf || count--)
		if (writeall(STDOUT_FILENO, buf, ptr))
			return free(buf), -1;
	return free(buf), 0;
}

int
main(int argc, char *argv[])
{
	UNOFLAGS(argc != 2);

	if ((inf = !strcmp(argv[0], "inf")))
		einf_check_fd(STDOUT_FILENO, "<stdout>");
	else
		count = etozu_arg("the count", argv[0], 0, SIZE_MAX);

	eopen_stream(&stream, !strcmp(argv[1], "-") ? NULL : argv[1]);
	if (count > SIZE_MAX / stream.frames)
		eprintf("%s: video is too long\n", stream.file);
	stream.frames *= count;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	if (!strcmp(argv[1], "-") ? repeat_stdin() : repeat_regular_file())
		if (!inf || errno != EPIPE)
			eprintf("write <stdout>:");

	close(stream.fd);
	return 0;
}
