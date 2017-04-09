/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <limits.h>
#include <unistd.h>

USAGE("[-i] file")

static void
to_stdout(struct stream *stream, size_t frame_size)
{
	size_t ptr, end, n;
	char buf[BUFSIZ];
	ssize_t r;

	while (stream->frames--) {
		ptr = stream->frames * frame_size + stream->headlen;
		end = ptr + frame_size;
		while (ptr < end) {
			if (!(r = epread(stream->fd, buf, sizeof(buf), ptr, stream->file)))
				eprintf("%s: file is shorter than expected\n", stream->file);
			ptr += n = (size_t)r;
			ewriteall(STDOUT_FILENO, buf, n, "<stdout>");
		}
	}
}

static void
elseek_set(int fd, off_t offset, const char *fname)
{
	off_t r = elseek(fd, offset, SEEK_SET, fname);
	if (r != offset)
		eprintf("%s: file is shorter than expected\n", fname);
}

static void
in_place(struct stream *stream, size_t frame_size)
{
	size_t f, fm = stream->frames - 1;
	off_t pa, pb;
	char *bufa, *bufb;

	bufa = emalloc(frame_size);
	bufb = emalloc(frame_size);

	for (f = 0; f < stream->frames >> 1; f++) {
		pa = f        * frame_size + stream->headlen;
		pb = (fm - f) * frame_size + stream->headlen;
		eread_frame(stream, bufa, frame_size);
		elseek_set(stream->fd, pb, stream->file);
		eread_frame(stream, bufb, frame_size);
		elseek_set(stream->fd, pb, stream->file);
		ewriteall(stream->fd, bufa, frame_size, stream->file);
		elseek_set(stream->fd, pa, stream->file);
		ewriteall(stream->fd, bufb, frame_size, stream->file);
	}

	free(bufa);
	free(bufb);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t frame_size;
	int inplace = 0;

	ARGBEGIN {
	case 'i':
		inplace = 1;
		break;
	default:
		usage();
	} ARGEND;
	
	if (argc != 1)
		usage();

	stream.file = argv[0];
	stream.fd = eopen(stream.file, inplace ? O_RDWR : O_RDONLY);
	einit_stream(&stream);
	if (!inplace) {
		fprint_stream_head(stdout, &stream);
		efflush(stdout, "<stdout>");
	}
	echeck_frame_size(stream.width, stream.height, stream.pixel_size, 0, stream.file);
	frame_size = stream.width * stream.height * stream.pixel_size;
	if (stream.frames > (size_t)SSIZE_MAX / frame_size ||
	    stream.frames * frame_size > (size_t)SSIZE_MAX - stream.headlen)
		eprintf("%s: video is too large\n", stream.file);

#if defined(POSIX_FADV_RANDOM)
	posix_fadvise(stream.fd, 0, 0, POSIX_FADV_RANDOM);
#endif

	(inplace ? in_place : to_stdout)(&stream, frame_size);
	close(stream.fd);
	return 0;
}
