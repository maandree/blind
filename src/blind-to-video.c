/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#if defined(HAVE_PRCTL)
# include <sys/prctl.h>
#endif
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("[-d] frame-rate ffmpeg-arguments ...")

static int draft = 0;

static void
process_xyza(char *buf, size_t n, int fd, const char *fname)
{
	double *pixel, r, g, b;
	uint16_t *pixels, *end;
	uint16_t pixbuf[1024];
	long int a, y, u, v;
	size_t ptr;
	pixels = pixbuf;
	end = pixbuf + ELEMENTSOF(pixbuf);
	if (draft) {
		for (ptr = 0; ptr < n; ptr += 4 * sizeof(double)) {
			pixel = (double *)(buf + ptr);
			ciexyz_to_scaled_yuv(pixel[0], pixel[1], pixel[2], &r, &g, &b);
			y = (long int)r +  16L * 256L;
			u = (long int)g + 128L * 256L;
			v = (long int)b + 128L * 256L;
			*pixels++ = 0xFFFFU;
			*pixels++ = htole16((uint16_t)(y < 0 ? 0 : y > 0xFFFFL ? 0xFFFFL : y));
			*pixels++ = htole16((uint16_t)(u < 0 ? 0 : u > 0xFFFFL ? 0xFFFFL : u));
			*pixels++ = htole16((uint16_t)(v < 0 ? 0 : v > 0xFFFFL ? 0xFFFFL : v));
			if (pixels == end)
				ewriteall(fd, pixels = pixbuf, sizeof(pixbuf), fname);
		}
	} else {
		for (ptr = 0; ptr < n; ptr += 4 * sizeof(double)) {
			pixel = (double *)(buf + ptr);
			a = (long int)(pixel[3] * 0xFFFFL);
			ciexyz_to_srgb(pixel[0], pixel[1], pixel[2], &r, &g, &b);
			r = srgb_encode(r);
			g = srgb_encode(g);
			b = srgb_encode(b);
			srgb_to_yuv(r, g, b, pixel + 0, pixel + 1, pixel + 2);
			y = (long int)(pixel[0] * 0xFFFFL) +  16L * 256L;
			u = (long int)(pixel[1] * 0xFFFFL) + 128L * 256L;
			v = (long int)(pixel[2] * 0xFFFFL) + 128L * 256L;
			*pixels++ = htole16((uint16_t)(a < 0 ? 0 : a > 0xFFFFL ? 0xFFFFL : a));
			*pixels++ = htole16((uint16_t)(y < 0 ? 0 : y > 0xFFFFL ? 0xFFFFL : y));
			*pixels++ = htole16((uint16_t)(u < 0 ? 0 : u > 0xFFFFL ? 0xFFFFL : u));
			*pixels++ = htole16((uint16_t)(v < 0 ? 0 : v > 0xFFFFL ? 0xFFFFL : v));
			if (pixels == end)
				ewriteall(fd, pixels = pixbuf, sizeof(pixbuf), fname);
		}
	}
	if (pixels != pixbuf)
		ewriteall(fd, pixbuf, (size_t)(pixels - pixbuf) * sizeof(*pixels), fname);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	char geometry[2 * 3 * sizeof(size_t) + 2];
	char *frame_rate;
	const char **cmd;
	size_t n = 0;
	int status, pipe_rw[2];
	pid_t pid;
	void (*process)(char *buf, size_t n, int fd, const char *fname) = NULL;

	ARGBEGIN {
	case 'd':
		draft = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc < 2)
		usage();

	frame_rate = *argv++, argc--;
	cmd = ecalloc((size_t)argc + 12, sizeof(*cmd));
	cmd[n++] = "ffmpeg";
	cmd[n++] = "-f",       cmd[n++] = "rawvideo";
	cmd[n++] = "-pix_fmt", cmd[n++] = "ayuv64le";
	cmd[n++] = "-r",       cmd[n++] = frame_rate;
	cmd[n++] = "-s:v",     cmd[n++] = geometry;
	cmd[n++] = "-i",       cmd[n++] = "-";
	memcpy(cmd + n, argv, (size_t)argc * sizeof(*cmd));

	stream.file = "<stdin>";
	stream.fd = STDIN_FILENO;
	einit_stream(&stream);

	sprintf(geometry, "%zux%zu", stream.width, stream.height);

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	if (pipe(pipe_rw))
		eprintf("pipe:");

	pid = fork();
	if (pid < 0)
		eprintf("fork:");

	if (!pid) {
#if defined(HAVE_PRCTL) && defined(PR_SET_PDEATHSIG)
		prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
		close(pipe_rw[1]);
		if (dup2(pipe_rw[0], STDIN_FILENO) == -1)
			eprintf("dup2:");
		close(pipe_rw[0]);
		execvp("ffmpeg", (char **)(void *)cmd);
		eprintf("exec ffmpeg:");
	}

	free(cmd);

	close(pipe_rw[0]);
	while (eread_stream(&stream, SIZE_MAX)) {
		n = stream.ptr - (stream.ptr % stream.pixel_size);
		process(stream.buf, n, pipe_rw[1], "<subprocess>");
		memmove(stream.buf, stream.buf + n, stream.ptr -= n);
	}
	close(pipe_rw[1]);

	if (waitpid(pid, &status, 0) == -1)
		eprintf("waitpid:");

	return !!status;
}
