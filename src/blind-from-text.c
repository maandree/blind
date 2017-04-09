/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("")

static void
process_xyza(void)
{
	double buf[BUFSIZ / sizeof(double)];
	size_t i;
	int r, done = 0;

	while (!done) {
		for (i = 0; i < ELEMENTSOF(buf); i += (size_t)r) {
			r = scanf("%lf", buf + i);
			if (r == EOF) {
				done = 1;
				break;
			}
		}
		ewriteall(STDOUT_FILENO, buf, i * sizeof(*buf), "<stdout>");
	}
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t size = 0;
	char *line = NULL;
	ssize_t len;
	void (*process)(void) = NULL;

	UNOFLAGS(argc);

	len = getline(&line, &size, stdin);
	if (len < 0) {
		if (ferror(stdin))
	  		eprintf("getline <stdin>:");
		else
			eprintf("<stdin>: no input\n");
	}
	if (len && line[len - 1] == '\n')
		line[--len] = '\0';
	if ((size_t)len + 6 > sizeof(stream.buf))
		eprintf("<stdin>: head is too long\n");
	stream.fd = -1;
	stream.file = "<stdin>";
	memcpy(stream.buf, line, (size_t)len);
	memcpy(stream.buf + len, "\n\0uivf", 6);
	stream.ptr = (size_t)len + 6;
	free(line);
	ewriteall(STDOUT_FILENO, stream.buf, stream.ptr, "<stdout>");
	einit_stream(&stream);

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	process();

	efshut(stdin, "<stdin>");
	return 0;
}
