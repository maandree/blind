/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#if defined(HAVE_PRCTL)
# include <sys/prctl.h>
#endif
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

USAGE("[-r frame-rate] [-w width -h height] [-d] input-file output-file")

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

	if (pipe(pipe_rw))
		eprintf("pipe:");

	pid = fork();
	if (pid == -1)
		eprintf("fork:");

	if (!pid) {
#if defined(HAVE_PRCTL) && defined(PR_SET_PDEATHSIG)
		prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
		fd = eopen(file, O_RDONLY);
		if (dup2(fd, STDIN_FILENO) == -1)
			eprintf("dup2:");
		close(fd);
		close(pipe_rw[0]);
		if (dup2(pipe_rw[1], STDOUT_FILENO) == -1)
			eprintf("dup2:");
		close(pipe_rw[1]);
		execlp("ffprobe", "ffprobe", "-v", "quiet", "-show_streams",
		       "-select_streams", "v", "-", NULL);
		eprintf("exec ffprobe:");
	}

	close(pipe_rw[1]);
	fp = fdopen(pipe_rw[0], "rb");
	if (!fp)
		eprintf("fdopen <subprocess>:");
	read_metadata(fp, file, width, height);
	fclose(fp);
	close(pipe_rw[0]);

	if (waitpid(pid, &status, 0) == -1)
		eprintf("waitpid:");
	if (status)
		exit(1);
}

static void
convert_segment(char *buf, size_t n, int fd, char *file)
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
convert(char *infile, int outfd, char *outfile, size_t width, size_t height, char *frame_rate)
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

	if (pipe(pipe_rw))
		eprintf("pipe:");

	pid = fork();
	if (pid == -1)
		eprintf("fork:");

	if (!pid) {
#if defined(HAVE_PRCTL) && defined(PR_SET_PDEATHSIG)
		prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
		close(pipe_rw[0]);
		if (dup2(pipe_rw[1], STDOUT_FILENO) == -1)
			eprintf("dup2:");
		close(pipe_rw[1]);
		execvp("ffmpeg", cmd);
		eprintf("exec ffmpeg:");
	}

	close(pipe_rw[1]);

	for (ptr = 0;;) {
		r = read(pipe_rw[0], buf + ptr, sizeof(buf) - ptr);
		if (r < 0)
			eprintf("read <subprocess>:");
		if (r == 0)
			break;
		ptr += (size_t)r;
		n = ptr - (ptr % 8);
		convert_segment(buf, n, outfd, outfile);
		memmove(buf, buf + n, ptr -= n);
	}
	if (ptr)
		eprintf("<subprocess>: incomplete frame\n");

	close(pipe_rw[0]);
	if (waitpid(pid, &status, 0) == -1)
		eprintf("waitpid:");
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
	char *outfile;
	char *data;
	ssize_t headlen;
	size_t length, frame_size;
	int outfd;
	struct stat st;

	ARGBEGIN {
	case 'd':
		draft = 1;
		break;
	case 'r':
		frame_rate = EARG();
		break;
	case 'w':
		width = etozu_flag('w', EARG(), 1, SIZE_MAX);
		break;
	case 'h':
		height = etozu_flag('h', EARG(), 1, SIZE_MAX);
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

	outfd = eopen(outfile, O_RDWR | O_CREAT | O_TRUNC, 0666);
	convert(infile, outfd, outfile, width, height, frame_rate);

	if (fstat(outfd, &st))
		eprintf("fstat %s:", outfile);
	length = (size_t)(st.st_size);

	if (length % frame_size)
		eprintf("<subprocess>: incomplete frame");
	frames = length / frame_size;

	sprintf(head, "%zu %zu %zu %s\n%cuivf%zn", frames, width, height, "xyza", 0, &headlen);
	ewriteall(outfd, head, (size_t)headlen, outfile);
	data = mmap(0, length + (size_t)headlen, PROT_READ | PROT_WRITE, MAP_SHARED, outfd, 0);
	memmove(data + headlen, data, length);
	memcpy(data, head, (size_t)headlen);
	munmap(data, length + (size_t)headlen);

	close(outfd);
	return 0;
}
