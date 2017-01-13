/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <arpa/inet.h>
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
read_metadata(FILE *fp, char *fname, size_t *width, size_t *height, size_t *frames)
{
	char *line = NULL;
	size_t size = 0;
	ssize_t len;
	int have_width = !width, have_height = !height, have_frames = 0;
	char *p;

	while ((len = getline(&line, &size, fp)) != -1) {
		if (len && line[len - 1])
			line[--len] = '\0';
		p = strchr(line, '=') + 1;
		if (width && strstr(line, "width=") == line) {
			if (tozu(p, 1, SIZE_MAX, width))
				eprintf("invalid width: %s\n", p);
			have_width = 1;
		} else if (height && strstr(line, "height=") == line) {
			if (tozu(p, 1, SIZE_MAX, height))
				eprintf("invalid height: %s\n", p);
			have_height = 1;
		} else if (strstr(line, "nb_read_frames=") == line) {
			if (tozu(p, 0, SIZE_MAX, frames))
				eprintf("invalid frame count: %s\n", p);
			have_frames = 1;
		}
	}

	if (ferror(fp))
		eprintf("getline %s:", fname);
	free(line);

	if (have_width + have_height + have_frames < 3)
		eprintf("could not get all required metadata\n");
}

static void
get_metadata(char *file, size_t *width, size_t *height, size_t *frames)
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
		       "-select_streams", "v", "-count_frames", "-", NULL);
		eprintf("exec ffprobe:");
	}

	close(pipe_rw[1]);
	fp = fdopen(pipe_rw[0], "rb");
	if (!fp)
		eprintf("fdopen <subprocess>:");
	read_metadata(fp, file, width, height, frames);
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
		for (ptr = i = 0; ptr < n; ptr += 8, i++) {
			pixels[i][3] = ntohs(((uint16_t *)(buf + ptr))[0]) / max;
			y = ntohs(((uint16_t *)(buf + ptr))[1]);
			u = ntohs(((uint16_t *)(buf + ptr))[2]);
			v = ntohs(((uint16_t *)(buf + ptr))[3]);
			scaled_yuv_to_ciexyz(y, u, v, pixels[i] + 0, pixels[i] + 1, pixels[i] + 2);
			if (++i == 1024) {
				i = 0;
				ewriteall(fd, pixels, sizeof(pixels), file);
			}
		}
	} else {
		for (ptr = i = 0; ptr < n; ptr += 8, i++) {
			pixels[i][3] = ntohs(((uint16_t *)(buf + ptr))[0]) / max;
			y = ntohs(((uint16_t *)(buf + ptr))[1]) / max;
			u = ntohs(((uint16_t *)(buf + ptr))[2]) / max;
			v = ntohs(((uint16_t *)(buf + ptr))[3]) / max;
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
	char geometry[2 * 3 * sizeof(size_t) + 2], *cmd[13], buf[BUFSIZ];
	int status, fd, pipe_rw[2];
	size_t i = 0, n, ptr;
	ssize_t r;
	pid_t pid;

	cmd[i++] = "ffmpeg";
	cmd[i++] = "-i", cmd[i++] = "-";
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
		fd = eopen(infile, O_RDONLY);
		if (dup2(fd, STDIN_FILENO) == -1)
			eprintf("dup2:");
		close(fd);
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
	size_t length;
	int outfd, status;
	pid_t pid;
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

	outfd = eopen(outfile, O_RDWR | O_CREAT | O_TRUNC, 0666);

	pid = fork();
	if (pid == -1)
		eprintf("fork:");

	if (!pid) {
#if defined(HAVE_PRCTL) && defined(PR_SET_PDEATHSIG)
		prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
		convert(infile, outfd, outfile, width, height, frame_rate);
		exit(0);
	}

	get_metadata(infile, width ? NULL : &width, height ? NULL : &height, &frames);

	if (waitpid(pid, &status, 0) == -1)
		eprintf("waitpid:");
	if (status)
		exit(1);

	if (fstat(outfd, &st))
		eprintf("fstat %s:", outfile);
	length = (size_t)(st.st_size);

	sprintf(head, "%zu %zu %zu %s\n%cuivf%zn", frames, width, height, "xyza", 0, &headlen);
	ewriteall(outfd, head, (size_t)headlen, outfile);
	data = mmap(0, length + (size_t)headlen, PROT_READ | PROT_WRITE, MAP_PRIVATE, outfd, 0);
	memmove(data + headlen, data, length);
	memcpy(data, head, (size_t)headlen);
	munmap(data, length + (size_t)headlen);

	close(outfd);
	return 0;
}
