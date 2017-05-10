/* See LICENSE file for copyright and license details. */
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#define STREAM_HEAD_MAX (3 * INTSTRLEN(size_t) + sizeof(((struct stream *)0)->pixfmt) + 10)

#define SPRINTF_HEAD_ZN(BUF, FRAMES, WIDTH, HEIGHT, PIXFMT, LENP)\
	sprintf(BUF, "%zu %zu %zu %s\n%cuivf%zn",\
		(size_t)(FRAMES), (size_t)(WIDTH), (size_t)(HEIGHT), PIXFMT, 0, LENP)

#define SPRINTF_HEAD(BUF, FRAMES, WIDTH, HEIGHT, PIXFMT)\
	sprintf(BUF, "%zu %zu %zu %s\n%cuivf",\
		(size_t)(FRAMES), (size_t)(WIDTH), (size_t)(HEIGHT), PIXFMT, 0)

#define FPRINTF_HEAD(FP, FRAMES, WIDTH, HEIGHT, PIXFMT)\
	fprintf(FP, "%zu %zu %zu %s\n%cuivf",\
		(size_t)(FRAMES), (size_t)(WIDTH), (size_t)(HEIGHT), PIXFMT, 0)

#define FPRINTF_HEAD_FMT(FP, FFRAMES, FRAMES, FWIDTH, WIDTH, FHEIGHT, HEIGHT, PIXFMT)\
	fprintf(FP, FFRAMES" "FWIDTH" "FHEIGHT" %s\n%cuivf",\
		FRAMES, WIDTH, HEIGHT, PIXFMT, 0)

#define einit_stream(...)             eninit_stream(1, __VA_ARGS__)
#define eopen_stream(...)             enopen_stream(1, __VA_ARGS__)
#define eset_pixel_size(...)          enset_pixel_size(1, __VA_ARGS__)
#define eread_stream(...)             enread_stream(1, __VA_ARGS__)
#define einf_check_fd(...)            eninf_check_fd(1, __VA_ARGS__)
#define echeck_dimensions(...)        encheck_dimensions(1, __VA_ARGS__)
#define echeck_dimensions_custom(...) encheck_dimensions_custom(1, __VA_ARGS__)
#define echeck_compat(...)            encheck_compat(1, __VA_ARGS__)
#define eread_segment(...)            enread_segment(1, __VA_ARGS__)
#define eread_frame(...)              enread_frame(1, __VA_ARGS__)
#define eread_row(...)                enread_row(1, __VA_ARGS__)

#define process_stream(...)                 nprocess_stream(1, __VA_ARGS__)
#define process_each_frame_segmented(...)   nprocess_each_frame_segmented(1, __VA_ARGS__)
#define process_two_streams(...)            nprocess_two_streams(1, __VA_ARGS__)
#define process_multiple_streams(...)       nprocess_multiple_streams(1, __VA_ARGS__)
#define process_each_frame_two_streams(...) nprocess_each_frame_two_streams(1, __VA_ARGS__)

enum dimension {
	WIDTH = 1,
	HEIGHT = 2,
	LENGTH = 4
};

struct stream {
	size_t frames;
	size_t width;
	size_t height;
	size_t pixel_size;
	char pixfmt[32];
	int fd;
#if INT_MAX != LONG_MAX
	int padding__;
#endif
	size_t ptr;
	size_t xptr;
	char buf[BUFSIZ];
	const char *file;
	size_t headlen;
	size_t row_size;
	size_t col_size;
	size_t frame_size;
};

void eninit_stream(int status, struct stream *stream);
void enopen_stream(int status, struct stream *stream, const char *file);
int set_pixel_size(struct stream *stream);
void enset_pixel_size(int status, struct stream *stream);
void fprint_stream_head(FILE *fp, struct stream *stream);
size_t enread_stream(int status, struct stream *stream, size_t n);
void eninf_check_fd(int status, int fd, const char *file);
void encheck_dimensions(int status, const struct stream *stream, enum dimension dimensions, const char *prefix);
void encheck_compat(int status, const struct stream *a, const struct stream *b);
const char *get_pixel_format(const char *specified, const char *current);
int enread_segment(int status, struct stream *stream, void *buf, size_t n);

void nprocess_stream(int status, struct stream *stream, void (*process)(struct stream *stream, size_t n));

void nprocess_each_frame_segmented(int status, struct stream *stream, int output_fd, const char* output_fname,
				   void (*process)(struct stream *stream, size_t n, size_t frame));

void nprocess_two_streams(int status, struct stream *left, struct stream *right, int output_fd, const char* output_fname,
			  void (*process)(struct stream *left, struct stream *right, size_t n));

void nprocess_multiple_streams(int status, struct stream *streams, size_t n_streams, int output_fd, const char* output_fname,
			       void (*process)(struct stream *streams, size_t n_streams, size_t n));

void nprocess_each_frame_two_streams(int status, struct stream *left, struct stream *right, int output_fd, const char* output_fname,
				     void (*process)(char *restrict output, char *restrict lbuf, char *restrict rbuf,
						     struct stream *left, struct stream *right));

static inline int
enread_frame(int status, struct stream *stream, void *buf)
{
	return enread_segment(status, stream, buf, stream->frame_size);
}

static inline int
enread_row(int status, struct stream *stream, void *buf)
{
	return enread_segment(status, stream, buf, stream->row_size);
}

static inline void
encheck_dimensions_custom(int status, size_t width, size_t height, size_t frames,
			  size_t pixel_size, const char *prefix, const char *fname)
{
	enum dimension dims = 0;
	struct stream stream;
	dims |= width  ? WIDTH  : 0;
	dims |= height ? HEIGHT : 0;
	dims |= frames ? LENGTH : 0;
	stream.width      = width;
	stream.height     = height;
	stream.frames     = frames;
	stream.pixel_size = pixel_size;
	stream.file       = fname;
	encheck_dimensions(status, &stream, dims, prefix);
}
