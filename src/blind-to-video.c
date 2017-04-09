/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

USAGE("[-d] frame-rate ffmpeg-arguments ...")

static int draft = 0;
static int fd;

static void
process_xyza(struct stream *stream, size_t n)
{
	char *buf = stream->buf;
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
			*pixels++ = htole16((uint16_t)CLIP(0, y, 0xFFFFL));
			*pixels++ = htole16((uint16_t)CLIP(0, u, 0xFFFFL));
			*pixels++ = htole16((uint16_t)CLIP(0, v, 0xFFFFL));
			if (pixels == end)
				ewriteall(fd, pixels = pixbuf, sizeof(pixbuf), "<subprocess>");
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
			*pixels++ = htole16((uint16_t)CLIP(0, a, 0xFFFFL));
			*pixels++ = htole16((uint16_t)CLIP(0, y, 0xFFFFL));
			*pixels++ = htole16((uint16_t)CLIP(0, u, 0xFFFFL));
			*pixels++ = htole16((uint16_t)CLIP(0, v, 0xFFFFL));
			if (pixels == end)
				ewriteall(fd, pixels = pixbuf, sizeof(pixbuf), "<subprocess>");
		}
	}
	ewriteall(fd, pixbuf, (size_t)(pixels - pixbuf) * sizeof(*pixels), "<subprocess>");
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
	void (*process)(struct stream *stream, size_t n) = NULL;

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

	eopen_stream(&stream, NULL);

	sprintf(geometry, "%zux%zu", stream.width, stream.height);

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	epipe(pipe_rw);
	pid = efork();

	if (!pid) {
		pdeath(SIGKILL);
		close(pipe_rw[1]);
		edup2(pipe_rw[0], STDIN_FILENO);
		close(pipe_rw[0]);
		eexecvp("ffmpeg", (char **)(void *)cmd);
	}

	free(cmd);

	close(pipe_rw[0]);
	fd = pipe_rw[1];
	process_stream(&stream, process);
	close(fd);

	ewaitpid(pid, &status, 0);
	return !!status;
}
