/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <string.h>
#include <unistd.h>

USAGE("")

static size_t
compare(const char *restrict new, const char *restrict old, size_t n, size_t **cmp, size_t *cmpsize)
{
	size_t i, start1, start2, ptr, same, diff;
	for (ptr = i = 0; i < n;) {
		for (start1 = i; i < n && old[i] == new[i]; i++);
		for (start2 = i; i < n && old[i] != new[i]; i++);
		same = start2 - start1;
		diff = i - start2;
		if (ptr && same < 2 * sizeof(size_t) && same + diff <= SIZE_MAX - (*cmp)[ptr - 1]) {
			(*cmp)[ptr - 1] += same + diff;
		} else {
			if (ptr + 2 > *cmpsize)
				*cmp = erealloc(*cmp, (*cmpsize += 128) * sizeof(size_t));
			(*cmp)[ptr++] = same;
			(*cmp)[ptr++] = diff;
		}
	}
	return ptr;
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	char *buf[2];
	size_t n, parts, part, off;
	int i;
	size_t *cmp = NULL;
	size_t cmpsize = 0;

	UNOFLAGS(argc);

	eopen_stream(&stream, NULL);
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, "<stdin>");
	n = stream.width * stream.height * stream.pixel_size;
	buf[0] = emalloc(n);
	buf[1] = ecalloc(1, n);

	for (i = 0; eread_frame(&stream, buf[i], n); i ^= 1) {
		parts = compare(buf[i], buf[i ^ 1], n, &cmp, &cmpsize);
		for (off = part = 0; part < parts; part += 2) {
			off += cmp[part];
			ewriteall(STDOUT_FILENO, cmp + part, 2 * sizeof(size_t), "<stdout>");
			ewriteall(STDOUT_FILENO, buf[i] + off, cmp[part + 1], "<stdout>");
			off += cmp[part + 1];
		}
	}

	free(cmp);
	free(buf[0]);
	free(buf[1]);
	return 0;
}
