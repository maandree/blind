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
int set_pixel_size(struct stream *stream);
void enset_pixel_size(int status, struct stream *stream);
void fprint_stream_head(FILE *fp, struct stream *stream);
size_t enread_stream(int status, struct stream *stream, size_t n);
void eninf_check_fd(int status, int fd, const char *file);
int check_frame_size(size_t width, size_t height, size_t pixel_size);
void encheck_frame_size(int status, size_t width, size_t height, size_t pixel_size, const char *prefix, const char *fname);
void encheck_compat(int status, const struct stream *a, const struct stream *b);
int enread_frame(int status, struct stream *stream, void *buf, size_t n);

#define EACH_FRAME_SEGMENTED(stream, process)\
	do {\
		size_t size__, f__, r__, n__;\
		echeck_frame_size((stream)->width, (stream)->height, (stream)->pixel_size, 0, (stream)->file);\
		size__ = (stream)->height * (stream)->width * (stream)->pixel_size;\
		for (f__ = 0; f__ < (stream)->frames; f__++) {\
			for (n__ = size__; n__; n__ -= r__) {\
				if (!eread_stream((stream), n__))\
					eprintf("%s: file is shorter than expected\n", (stream)->file);\
				r__ = (stream)->ptr - ((stream)->ptr % (stream)->pixel_size);\
				(process)((stream), r__, f__);\
				ewriteall(STDOUT_FILENO, (stream)->buf, r__, "<stdout>");\
				memmove((stream)->buf, (stream)->buf + r__, (stream)->ptr -= r__);\
			}\
		}\
	} while (0)
