/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

USAGE("[-o output-file] first-stream ... last-stream")

static void
concat_to_stdout(int argc, char *argv[])
{
	struct stream *streams;
	size_t frames = 0;
	int i;

	streams = emalloc((size_t)argc * sizeof(*streams));

	for (i = 0; i < argc; i++) {
		streams[i].file = argv[i];
		streams[i].fd = eopen(streams[i].file, O_RDONLY);
		einit_stream(streams + i);
		if (i)
			echeck_compat(streams + i, streams);
		if (streams[i].frames > SIZE_MAX - frames)
			eprintf("resulting video is too long\n");
		frames += streams[i].frames;
	}

	streams->frames = frames;
	fprint_stream_head(stdout, streams);
	efflush(stdout, "<stdout>");

	for (i = 0; i < argc; i++) {
		for (; eread_stream(streams + i, SIZE_MAX); streams[i].ptr = 0)
			ewriteall(STDOUT_FILENO, streams[i].buf, streams[i].ptr, "<stdout>");
		close(streams[i].fd);
	}

	free(streams);
}

static void
concat_to_file(int argc, char *argv[], char *output_file)
{
	struct stream stream, refstream;
	int first = 1;
	int fd = eopen(output_file, O_RDWR | O_CREAT | O_TRUNC, 0666);
	char head[STREAM_HEAD_MAX];
	ssize_t headlen, size = 0;
	char *data;

	for (; argc--; argv++) {
		stream.file = *argv;
		stream.fd = eopen(stream.file, O_RDONLY);
		einit_stream(&stream);

		if (first) {
			refstream = stream;
			first = 1;
		} else {
			if (refstream.frames > SIZE_MAX - stream.frames)
				eprintf("resulting video is too long\n");
			refstream.frames += stream.frames;
			echeck_compat(&stream, &refstream);
		}

		for (; eread_stream(&stream, SIZE_MAX); stream.ptr = 0) {
			ewriteall(fd, stream.buf, stream.ptr, output_file);
			size += stream.ptr;
		}
		close(stream.fd);
	}

	sprintf(head, "%zu %zu %zu %s\n%cuivf%zn",
		stream.frames, stream.width, stream.height, stream.pixfmt, 0, &headlen);

	ewriteall(fd, head, (size_t)headlen, output_file);

	data = mmap(0, size + (size_t)headlen, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED)
		eprintf("mmap %s:", output_file);
	memmove(data + headlen, data, size);
	memcpy(data, head, (size_t)headlen);
	munmap(data, size + (size_t)headlen);

	close(fd);
}

int
main(int argc, char *argv[])
{
	char *output_file = NULL;

	ARGBEGIN {
	case 'o':
		output_file = EARG();
		break;
	default:
		usage();
	} ARGEND;

	if (argc < 2)
		usage();

	if (output_file)
		concat_to_file(argc, argv, output_file);
	else
		concat_to_stdout(argc, argv);

	return 0;
}
