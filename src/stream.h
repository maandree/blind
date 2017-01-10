/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdio.h>

struct stream
{
	size_t frames;
	size_t width;
	size_t height;
	size_t pixel_size;
	char pixfmt[32];
	int fd;
	size_t ptr;
	char buf[4096];
	const char *file;
};


void eninit_stream(int status, struct stream *stream);

static inline void
einit_stream(struct stream *stream)
{
	eninit_stream(1, stream);
}


int set_pixel_size(struct stream *stream);
void enset_pixel_size(int status, struct stream *stream);

static inline void
eset_pixel_size(struct stream *stream)
{
	enset_pixel_size(1, stream);
}


void fprint_stream_head(FILE *fp, struct stream *stream);


size_t enread_stream(int status, struct stream *stream, size_t n);

static inline size_t
eread_stream(struct stream *stream, size_t n)
{
	return enread_stream(1, stream, n);
}


void eninf_check_fd(int status, int fd, const char *file);

static inline void
einf_check_fd(int fd, const char *file)
{
	eninf_check_fd(1, fd, file);
}
