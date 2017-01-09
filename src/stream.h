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

void einit_stream(struct stream *stream);
void eninit_stream(int status, struct stream *stream);

int set_pixel_size(struct stream *stream);
void eset_pixel_size(struct stream *stream);
void enset_pixel_size(int status, struct stream *stream);

void fprint_stream_head(FILE *fp, struct stream *stream);

size_t eread_stream(struct stream *stream, size_t n);
size_t enread_stream(int status, struct stream *stream, size_t n);
