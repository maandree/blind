/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <sys/stat.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

	if (!stream->width)
		eprintf("%s: width is zero\n", stream->file);
	if (!stream->height)
		eprintf("%s: height is zero\n", stream->file);

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


size_t
enread_stream(int status, struct stream *stream, size_t n)
{
	ssize_t r = read(stream->fd, stream->buf + stream->ptr,
			 sizeof(stream->buf) - stream->ptr < n ?
			 sizeof(stream->buf) - stream->ptr : n);
	if (r < 0)
		enprintf(status, "read %s:", stream->file);
	stream->ptr += (size_t)r;
	return (size_t)r;
}


void
eninf_check_fd(int status, int fd, const char *file)
{
	struct stat st;
	if (fstat(fd, &st))
		enprintf(status, "fstat %s:", file);
	if (S_ISREG(st.st_mode))
		enprintf(status, "%s is a regular file, refusing infinite write\n");
}


int
check_frame_size(size_t width, size_t height, size_t pixel_size)
{
	if (!width || !height || !pixel_size)
		return 1;
	if (width > SIZE_MAX / height)
		return 0;
	if (width * height > SIZE_MAX / pixel_size)
		return 0;
	return 1;
}

void
encheck_frame_size(int status, size_t width, size_t height, size_t pixel_size, const char *prefix, const char *fname)
{
	if (!check_frame_size(width, height, pixel_size))
		enprintf(status, "%s: %s%svideo frame is too large\n",
			 prefix ? prefix : "", (prefix && *prefix) ? " " : "", fname);
}


void
encheck_compat(int status, const struct stream *a, const struct stream *b)
{
	if (a->width != b->width || a->height != b->height)
		eprintf("videos do not have the same geometry\n");
	if (strcmp(a->pixfmt, b->pixfmt))
		eprintf("videos use incompatible pixel formats\n");
}


int
enread_frame(int status, struct stream *stream, void *buf, size_t n)
{
	char *buffer = buf;
	ssize_t r;
	for (; stream->ptr < n; stream->ptr += (size_t)r) {
		r = read(stream->fd, buffer + stream->ptr, n - stream->ptr);
		if (r < 0) {
			eprintf("read %s:", stream->file);
		} else if (r == 0) {
			if (!stream->ptr)
				break;
			eprintf("%s: incomplete frame", stream->file);
		}
	}
	if (!stream->ptr)
		return 0;
	stream->ptr = 0;
	return 1;
}
