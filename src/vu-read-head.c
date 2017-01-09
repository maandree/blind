/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <ctype.h>
#include <unistd.h>

static void
usage(void)
{
	eprintf("usage: %s\n", argv0);
}

int
main(int argc, char *argv[])
{
	char buf[2 + 3 * sizeof(size_t) + sizeof(((struct stream *)0)->pixfmt)];
	char magic[] = {'\0', 'u', 'i', 'v', 'f'};
	char b;
	char *p;
	size_t i, ptr;
	ssize_t r;

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	if (!argc)
		usage();

	for (ptr = 0; ptr < sizeof(buf);) {
		r = read(STDIN_FILENO, buf + ptr, 1);
		if (r < 0)
			eprintf("read <stdin>:");
		if (r == 0)
			goto bad_format;
		if (buf[ptr++] == '\n')
			break;
	}
	if (ptr == sizeof(buf))
		goto bad_format;

	p = buf;
	for (i = 0; i < 5; i++) {
		r = read(STDIN_FILENO, &b, 1);
		if (r < 0)
			eprintf("read <stdin>:");
		if (!r || b != magic[i])
			goto bad_format;
	}

	for (i = 0; i < 2; i++) {
		if (!isdigit(*p))
			goto bad_format;
		while (isdigit(*p)) p++;
		if (*p++ != ' ')
			goto bad_format;
	}
	while (isalnum(*p) || *p == ' ') {
		if (p[0] == ' ' && p[-1] == ' ')
			goto bad_format;
		p++;
	}
	if (p[-1] == ' ' || p[0] != '\n')
		goto bad_format;

	for (i = 0; i < ptr; i += (size_t)r) {
		r = write(STDOUT_FILENO, buf + i, ptr - i);
		if (r < 0)
			eprintf("write <stdout>:");
	}

	return 0;
bad_format:
	eprintf("<stdin>: file format not supported\n");
}
