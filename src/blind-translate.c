/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

USAGE("[-wp] translation-stream")

static int invtrans = 0;
static char zeroes[BUFSIZ];

static void*
next_pixel(struct stream *stream, size_t *ptr)
{
	void *ret;
	if (*ptr + stream->pixel_size >= stream->ptr) {
		memmove(stream->buf, stream->buf + *ptr, stream->ptr -= *ptr);
		*ptr = 0;
		while (stream->ptr < stream->pixel_size)
			if (!eread_stream(stream, SIZE_MAX))
				return NULL;
	}
	ret = stream->buf + *ptr;
	*ptr += stream->pixel_size;
	return ret;
}

static int
process_frame(struct stream *stream, char *buf, size_t n,
	      size_t above, size_t below, size_t left, size_t right)
{
#define ZEROES(N) ewritezeroes(STDOUT_FILENO, zeroes, sizeof(zeroes), N, "<stdout>")

	size_t i, w = n - left - right;
	int first = 1;

	if (!eread_row(stream, buf, n))
		return 0;

	for (i = 0; i < above; i++)
		ZEROES(n);
	for (i = 0; i < below; i++, first = 0)
		if (!first && !eread_row(stream, buf, n))
			goto eof;

	for (i = above + below; i < stream->height; i++, first = 0) {
		if (!first && !eread_row(stream, buf, n))
			goto eof;
		ZEROES((size_t)left);
		ewriteall(STDOUT_FILENO, buf + right, w, "<stdout>");
		ZEROES((size_t)right);
	}

	for (i = 0; i < below; i++)
		ZEROES(n);
	for (i = 0; i < above; i++, first = 0)
		if (!first && !eread_row(stream, buf, n))
			goto eof;

	return 1;
eof:
	eprintf("%s: file is shorter than expected\n", stream->file);
	return 0;
}

static void
process(struct stream *stream, struct stream *trstream)
{
	char *buf;
	size_t n, p = 0;
	double *trans, tmp;
	ssize_t trx = 0, try = 0;
	size_t above = 0, below = 0, left = 0, right = 0;

	memset(zeroes, 0, sizeof(zeroes));

	echeck_frame_size(stream->width, 1, stream->pixel_size, 0, stream->file);
	n = stream->width * stream->pixel_size;
	buf = emalloc(n);

	do {
		if ((trans = next_pixel(trstream, &p))) {
			trx = (ssize_t)(tmp = round(invtrans ? -trans[0] : trans[0]));
			try = (ssize_t)(tmp = round(invtrans ? -trans[1] : trans[1]));

			above =  try > 0 ? (size_t)try  : 0;
			below =  try < 0 ? (size_t)-try : 0;
			left  = (trx > 0 ? (size_t)trx  : 0) * stream->pixel_size;
			right = (trx < 0 ? (size_t)-trx : 0) * stream->pixel_size;

			above = MIN(above, stream->height);
			below = MIN(below, stream->height);
			left  = MIN(left,  n);
			right = MIN(right, n);
		}
	} while (process_frame(stream, buf, n, above, below, left, right));

	free(buf);
}

static void
process_wrap(struct stream *stream, struct stream *trstream)
{
	char *buf, *row;
	size_t n, rown, p = 0;
	double *trans, tmp;
	ssize_t trx = 0, try = 0, py;
	size_t off = 0, y;

	echeck_frame_size(stream->width, stream->height, stream->pixel_size, 0, "<stdin>");
	n = stream->height * (rown = stream->width * stream->pixel_size);
	buf = emalloc(n);

	while (eread_frame(stream, buf, n)) {
		if ((trans = next_pixel(trstream, &p))) {
			trx = (ssize_t)(tmp = round(invtrans ? -trans[0] : trans[0]));
			try = (ssize_t)(tmp = round(invtrans ? -trans[1] : trans[1]));
			trx %= (ssize_t)stream->width;
			if (trx < 0)
				trx += (ssize_t)stream->width;
			off = (stream->width - (size_t)trx) % stream->width;
			off = off * stream->pixel_size;
		}

		for (y = 0; y < stream->height; y++) {
			py = ((ssize_t)y - try) % (ssize_t)stream->height;
			if (py < 0)
				py += (ssize_t)stream->height;
			row = buf + (size_t)py * rown;
			ewriteall(STDOUT_FILENO, row + off, rown - off, "<stdout>");
			ewriteall(STDOUT_FILENO, row, off, "<stdout>");
		}
	}

	free(buf);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	struct stream trstream;
	int wrap = 0;

	ARGBEGIN {
	case 'w':
		wrap = 1;
		break;
	case 'p':
		invtrans = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 1)
		usage();

	eopen_stream(&stream, NULL);
	eopen_stream(&trstream, argv[0]);

	if (trstream.width != 1 || trstream.height != 1)
		eprintf("translation-stream does not have 1x1 geometry\n");

	if (strcmp(trstream.pixfmt, "xyza"))
		eprintf("pixel format of translation-stream %s "
			"is not supported, try xyza\n", trstream.pixfmt);

	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

	(wrap ? process_wrap : process)(&stream, &trstream);
	close(trstream.fd);
	return 0;
}
