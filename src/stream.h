/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdio.h>

#define einit_stream(...)      eninit_stream(1, __VA_ARGS__)
#define eset_pixel_size(...)   enset_pixel_size(1, __VA_ARGS__)
#define eread_stream(...)      enread_stream(1, __VA_ARGS__)
#define einf_check_fd(...)     eninf_check_fd(1, __VA_ARGS__)
#define echeck_frame_size(...) encheck_frame_size(1, __VA_ARGS__)
#define echeck_compat(...)     encheck_compat(1, __VA_ARGS__)
#define eread_frame(...)       enread_frame(1, __VA_ARGS__)

#define enread_row(...) enread_frame(__VA_ARGS__)
#define eread_row(...)  eread_frame(__VA_ARGS__)

#define process_each_frame_segmented(...)   nprocess_each_frame_segmented(1, __VA_ARGS__)
#define process_two_streams(...)            nprocess_two_streams(1, __VA_ARGS__)
#define process_multiple_streams(...)       nprocess_multiple_streams(1, __VA_ARGS__)
#define process_each_frame_two_streams(...) nprocess_each_frame_two_streams(1, __VA_ARGS__)

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
	size_t headlen;
};

void eninit_stream(int status, struct stream *stream);
int set_pixel_size(struct stream *stream);
void enset_pixel_size(int status, struct stream *stream);
void fprint_stream_head(FILE *fp, struct stream *stream);
size_t enread_stream(int status, struct stream *stream, size_t n);
void eninf_check_fd(int status, int fd, const char *file);
int check_frame_size(size_t width, size_t height, size_t pixel_size);
void encheck_frame_size(int status, size_t width, size_t height, size_t pixel_size, const char *prefix, const char *fname);
void encheck_compat(int status, const struct stream *a, const struct stream *b);
int enread_frame(int status, struct stream *stream, void *buf, size_t n);

void nprocess_each_frame_segmented(int status, struct stream *stream, int output_fd, const char* output_fname,
				  void (*process)(struct stream *stream, size_t n, size_t frame));

void nprocess_two_streams(int status, struct stream *left, struct stream *right, int output_fd, const char* output_fname,
			 void (*process)(struct stream *left, struct stream *right, size_t n));

void nprocess_multiple_streams(int status, struct stream *streams, size_t n_streams, int output_fd, const char* output_fname,
			      void (*process)(struct stream *streams, size_t n_streams, size_t n));

void nprocess_each_frame_two_streams(int status, struct stream *left, struct stream *right, int output_fd, const char* output_fname,
				     void (*process)(char *restrict output, char *restrict lbuf, char *restrict rbuf,
						     struct stream *left, struct stream *right, size_t ln, size_t rn));
