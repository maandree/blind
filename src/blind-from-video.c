/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

USAGE("[-r frame-rate] [-w width -h height] [-dL] input-file output-file")

static int draft = 0;

static void
read_metadata(FILE *fp, char *fname, size_t *width, size_t *height)
{
	char *line = NULL;
	size_t size = 0;
	ssize_t len;
	char *p;

	while ((len = getline(&line, &size, fp)) != -1) {
		if (len && line[len - 1])
			line[--len] = '\0';
		p = strchr(line, '=') + 1;
		if (strstr(line, "width=") == line) {
			if (tozu(p, 1, SIZE_MAX, width))
				eprintf("invalid width: %s\n", p);
		} else if (strstr(line, "height=") == line) {
			if (tozu(p, 1, SIZE_MAX, height))
				eprintf("invalid height: %s\n", p);
		}
	}

	if (ferror(fp))
		eprintf("getline %s:", fname);
	free(line);

	if (!*width || !*height)
		eprintf("could not get all required metadata\n");
}

static void
get_metadata(char *file, size_t *width, size_t *height)
{
	FILE *fp;
	int fd, pipe_rw[2];
	pid_t pid;
	int status;

	epipe(pipe_rw);
	pid = efork();

	if (!pid) {
		pdeath(SIGKILL);
		fd = eopen(file, O_RDONLY);
		edup2(fd, STDIN_FILENO);
		close(fd);
		close(pipe_rw[0]);
		edup2(pipe_rw[1], STDOUT_FILENO);
		close(pipe_rw[1]);
		eexeclp("ffprobe", "ffprobe", "-v", "quiet", "-show_streams",
			"-select_streams", "v", "-", NULL);
	}

	close(pipe_rw[1]);
	fp = fdopen(pipe_rw[0], "rb");
	if (!fp)
		eprintf("fdopen <subprocess>:");
	read_metadata(fp, file, width, height);
	fclose(fp);
	close(pipe_rw[0]);

	ewaitpid(pid, &status, 0);
	if (status)
		exit(1);
}

static void
convert_segment(char *buf, size_t n, int fd, const char *file)
{
	typedef double pixel_t[4];
	size_t i, ptr;
	double y, u, v, max = (double)UINT16_MAX;
	double r, g, b;
	pixel_t pixels[1024];
	if (draft) {
		for (ptr = i = 0; ptr < n; ptr += 8) {
			pixels[i][3] = 1;
			y = (long int)(le16toh(((uint16_t *)(buf + ptr))[1])) -  16L * 256L;
			u = (long int)(le16toh(((uint16_t *)(buf + ptr))[2])) - 128L * 256L;
			v = (long int)(le16toh(((uint16_t *)(buf + ptr))[3])) - 128L * 256L;
			scaled_yuv_to_ciexyz(y, u, v, pixels[i] + 0, pixels[i] + 1, pixels[i] + 2);
			if (++i == 1024) {
				i = 0;
				ewriteall(fd, pixels, sizeof(pixels), file);
			}
		}
	} else {
		for (ptr = i = 0; ptr < n; ptr += 8) {
			pixels[i][3] = le16toh(((uint16_t *)(buf + ptr))[0]) / max;
			y = ((long int)le16toh(((uint16_t *)(buf + ptr))[1]) -  16L * 256L) / max;
			u = ((long int)le16toh(((uint16_t *)(buf + ptr))[2]) - 128L * 256L) / max;
			v = ((long int)le16toh(((uint16_t *)(buf + ptr))[3]) - 128L * 256L) / max;
			yuv_to_srgb(y, u, v, &r, &g, &b);
			r = srgb_decode(r);
			g = srgb_decode(g);
			b = srgb_decode(b);
			srgb_to_ciexyz(r, g, b, pixels[i] + 0, pixels[i] + 1, pixels[i] + 2);
			if (++i == 1024) {
				i = 0;
				ewriteall(fd, pixels, sizeof(pixels), file);
			}
		}
	}
	if (i)
		ewriteall(fd, pixels, i * sizeof(*pixels), file);
}

static void
convert(const char *infile, int outfd, const char *outfile, size_t width, size_t height, const char *frame_rate)
{
	char geometry[2 * 3 * sizeof(size_t) + 2], buf[BUFSIZ];
	const char *cmd[13];
	int status, pipe_rw[2];
	size_t i = 0, n, ptr;
	ssize_t r;
	pid_t pid;

	cmd[i++] = "ffmpeg";
	cmd[i++] = "-i", cmd[i++] = infile;
	cmd[i++] = "-f", cmd[i++] = "rawvideo";
	cmd[i++] = "-pix_fmt", cmd[i++] = "ayuv64le";
	if (width && height) {
		sprintf(geometry, "%zux%zu", width, height);
		cmd[i++] = "-s:v", cmd[i++] = geometry;
	}
	if (frame_rate)
		cmd[i++] = "-r", cmd[i++] = frame_rate;
	cmd[i++] = "-";
	cmd[i++] = NULL;

	epipe(pipe_rw);
	pid = efork();

	if (!pid) {
		pdeath(SIGKILL);
		close(pipe_rw[0]);
		edup2(pipe_rw[1], STDOUT_FILENO);
		close(pipe_rw[1]);
		eexecvp("ffmpeg", (char **)(void *)cmd);
	}

	close(pipe_rw[1]);

	for (ptr = 0;;) {
		if (!(r = eread(pipe_rw[0], buf + ptr, sizeof(buf) - ptr, "<subprocess>")))
			break;
		ptr += (size_t)r;
		n = ptr - (ptr % 8);
		convert_segment(buf, n, outfd, outfile);
		memmove(buf, buf + n, ptr -= n);
	}
	if (ptr)
		eprintf("<subprocess>: incomplete frame\n");

	close(pipe_rw[0]);
	ewaitpid(pid, &status, 0);
	if (status)
		exit(1);
}

int
main(int argc, char *argv[])
{
	size_t width = 0, height = 0, frames;
	char head[STREAM_HEAD_MAX];
	char *frame_rate = NULL;
	char *infile;
	const char *outfile;
	char *data;
	ssize_t headlen;
	size_t length, frame_size;
	int outfd, skip_length = 0;
	struct stat st;

	ARGBEGIN {
	case 'd':
		draft = 1;
		break;
	case 'L':
		skip_length = 1;
		break;
	case 'r':
		frame_rate = UARGF();
		break;
	case 'w':
		width = etozu_flag('w', UARGF(), 1, SIZE_MAX);
		break;
	case 'h':
		height = etozu_flag('h', UARGF(), 1, SIZE_MAX);
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 2 || !width != !height)
		usage();

	infile = argv[0];
	outfile = argv[1];

	if (!width)
		get_metadata(infile, &width, &height);
	if (width > SIZE_MAX / height)
		eprintf("video frame too large\n");
	frame_size = width * height;
	if (4 * sizeof(double) > SIZE_MAX / frame_size)
		eprintf("video frame too large\n");
	frame_size *= 4 * sizeof(double);

	if (!strcmp(outfile, "-")) {
		outfile = "<stdout>";
		outfd = STDOUT_FILENO;
		if (!skip_length)
			eprintf("standard out as output file is only allowed with -L\n");
	} else {
		outfd = eopen(outfile, O_RDWR | O_CREAT | O_TRUNC, 0666);
	}

	if (skip_length) {
		SPRINTF_HEAD_ZN(head, frames, width, height, "xyza", &headlen);
		ewriteall(outfd, head, (size_t)headlen, outfile);
	}

	convert(infile, outfd, outfile, width, height, frame_rate);

	if (fstat(outfd, &st))
		eprintf("fstat %s:", outfile);
	length = (size_t)(st.st_size);

	if (length % frame_size)
		eprintf("<subprocess>: incomplete frame");
	frames = length / frame_size;

	if (!skip_length) {
		SPRINTF_HEAD_ZN(head, frames, width, height, "xyza", &headlen);
		ewriteall(outfd, head, (size_t)headlen, outfile);
		data = mmap(0, length + (size_t)headlen, PROT_READ | PROT_WRITE, MAP_SHARED, outfd, 0);
		memmove(data + headlen, data, length);
		memcpy(data, head, (size_t)headlen);
		munmap(data, length + (size_t)headlen);
	}

	close(outfd);
	return 0;
}
