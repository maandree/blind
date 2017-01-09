/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void
einit_stream(struct stream *stream)
{
	eninit_stream(1, stream);
}

void
eninit_stream(int status, struct stream *stream)
{
	ssize_t r;
	size_t n;
	char *p = NULL, *w, *h, *f, *end;

	for (stream->ptr = 0; p;) {
		r = read(stream->fd, stream->buf + stream->ptr, sizeof(stream->buf) - stream->ptr);
		if (r < 0)
			enprintf(status, "read %s:", stream->file);
		if (r == 0)
			goto bad_format;
		stream->ptr += (size_t)r;
		p = memchr(stream->buf, '\n', stream->ptr);
	}

	*p = '\0';
	w = strchr(stream->buf, ' ');
	if (!w)
		goto bad_format;
	h = strchr(w + 1, ' ');
	if (!h)
		goto bad_format;
	f = strchr(h + 1, ' ');
	if (!f)
		goto bad_format;
	*w++ = *h++ = *f++ = '\0';

	if (strlen(f) >= sizeof(stream->pixfmt))
		goto bad_format;
	strcpy(stream->pixfmt, f);
	errno = 0;
	stream->frames = strtoul(stream->buf, &end, 10);
	if (errno == ERANGE && *stream->buf != '-')
		eprintf("%s: too long\n", stream->file);
	if (errno || *end)
		goto bad_format;
	errno = 0;
	stream->width = strtoul(w, &end, 10);
	if (errno == ERANGE && *stream->buf != '-')
		eprintf("%s: too wide\n", stream->file);
	if (errno || *end)
		goto bad_format;
	errno = 0;
	stream->height = strtoul(h, &end, 10);
	if (errno == ERANGE && *stream->buf != '-')
		eprintf("%s: too tall\n", stream->file);
	if (errno || *end)
		goto bad_format;

	n = (size_t)(p - stream->buf) + 1;
	memmove(stream->buf, stream->buf + n, stream->ptr -= n);
	while (stream->ptr < 5) {
		r = read(stream->fd, stream->buf + stream->ptr, sizeof(stream->buf) - stream->ptr);
		if (r < 0)
			enprintf(status, "read %s:", stream->file);
		if (r == 0)
			goto bad_format;
		stream->ptr += (size_t)r;
	}
	if (stream->buf[0] != '\0' ||
	    stream->buf[1] != 'u' || stream->buf[2] != 'i' ||
	    stream->buf[3] != 'v' || stream->buf[4] != 'f')
		goto bad_format;
	memmove(stream->buf, stream->buf + 5, stream->ptr -= 5);

	enset_pixel_size(status, stream);

	return;
bad_format:
	enprintf(status, "%s: file format not supported%s\n", stream->file);
}


int
set_pixel_size(struct stream *stream)
{
	if (!strcmp(stream->pixfmt, "xyza"))
		stream->pixel_size = 4 * sizeof(double);
	else
		return -1;
	return 0;
}

void
eset_pixel_size(struct stream *stream)
{
	enset_pixel_size(1, stream);
}

void
enset_pixel_size(int status, struct stream *stream)
{
	if (set_pixel_size(stream))
		enprintf(status, "file %s uses unsupported pixel format: %s\n",
			 stream->file, stream->pixfmt);
}


void
fprint_stream_head(FILE *fp, struct stream *stream)
{
	fprintf(fp, "%zu %zu %zu %s\n%cuivf",
		stream->frames, stream->width, stream->height, stream->pixfmt, 0);
}


size_t eread_stream(struct stream *stream, size_t n)
{
	return enread_stream(1, stream, n);
}

size_t enread_stream(int status, struct stream *stream, size_t n)
{
	ssize_t r = read(stream->fd, stream->buf + stream->ptr,
			 sizeof(stream->buf) - stream->ptr < n ?
			 sizeof(stream->buf) - stream->ptr : n);
	if (r < 0)
		enprintf(status, "read %s:", stream->file);
	stream->ptr += (size_t)r;
	return (size_t)r;
}
