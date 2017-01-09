/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int luma_warning_triggered = 0;
static int gamut_warning_triggered = 0;
static int alpha_warning_triggered = 0;

static void
usage(void)
{
	eprintf("usage: %s [-d depth]\n", argv0);
}

static void
write_pixel(double R, double G, double B, double A, int bytes, unsigned long long int max)
{
	unsigned long long int colours[4];
	unsigned char buf[4 * 8];
	int i, j, k, bm = bytes - 1;
	size_t ptr, n;
	ssize_t r;

	if (R < 0 || G < 0 || B < 0 || R > 1 || G > 1 || B > 1) {
		if (gamut_warning_triggered) {
			gamut_warning_triggered = 1;
			weprintf("warning: out-of-gamut colour detected\n");
		}
		; /* TODO gamut */
		R = R < 0 ? 0 : R > 1 ? 1 : R;
		G = G < 0 ? 0 : G > 1 ? 1 : G;
		B = B < 0 ? 0 : B > 1 ? 1 : B;
	}

	if (A < 0 || A > 1) {
		if (alpha_warning_triggered) {
			alpha_warning_triggered = 1;
			weprintf("warning: alpha values truncated\n");
		}
		A = A < 0 ? 0 : 1;
	}

	colours[0] = srgb_encode(R) * max + 0.5;
	colours[1] = srgb_encode(G) * max + 0.5;
	colours[2] = srgb_encode(B) * max + 0.5;
	colours[3] = A * max + 0.5;

	for (i = k = 0; i < 4; i++) {
		for (j = 0; j < bytes; j++, k++) {
			buf[k + bm - j] = (unsigned char)(colours[j]);
			colours[j] >>= 8;
		}
	}

	n = (size_t)bytes * 4;
	for (ptr = 0; ptr < n; ptr += (size_t)r) {
		r = write(STDOUT_FILENO, buf + ptr, n - ptr);
		if (r < 0)
			eprintf("write <stdout>:");
	}
}

static void
process_xyza(struct stream *stream, size_t n, int bytes, unsigned long long int max)
{
	size_t i;
	double X, Y, Z, A, R, G, B;
	for (i = 0; i < n; i += stream->pixel_size) {
		X = ((double *)(stream->buf + i))[0];
		Y = ((double *)(stream->buf + i))[1];
		Z = ((double *)(stream->buf + i))[2];
		A = ((double *)(stream->buf + i))[3];

		if (Y < 0 || Y > 1) {
			if (luma_warning_triggered) {
				luma_warning_triggered = 1;
				weprintf("warning: %s colour detected\n",
					 Y < 0 ? "subblack" : "superwhite");
			}
		}

		srgb_to_ciexyz(X, Y, Z, &R, &G, &B);
		write_pixel(R, G, B, A, bytes, max);
	}
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	int depth = 16, bytes;
	unsigned long long int max;
	size_t n;
	void (*process)(struct stream *stream, size_t n, int bytes, unsigned long long int max);

	ARGBEGIN {
	case 'd':
		if (toi(EARGF(usage()), 1, 64, &depth))
			eprintf("argument of -d must be an integer in [1, 64]\n");
		break;
	default:
		usage();
	} ARGEND;

	if (argc)
		usage();

	stream.fd = STDIN_FILENO;
	stream.file = "<stdin.h>";
	einit_stream(&stream);

	max = 1ULL << (depth - 1);
	max |= max - 1;
	for (bytes = 1; bytes * 8 < depth; bytes++);

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	printf("P7\n"
	       "WIDTH %zu\n"
	       "HEIGHT %zu\n"
	       "DEPTH 4\n" /* Depth actually means channels */
	       "MAXVAL %llu\n"
	       "TUPLTYPE RGB_ALPHA\n"
	       "ENDHDR\n", stream.width, stream.height, max);
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");

	for (;;) {
		n = stream.ptr;
		n -= n % stream.pixel_size;
		process(&stream, n, bytes, max);
		memmove(stream.buf, stream.buf + n, stream.ptr -= n);
		if (!eread_stream(&stream, SIZE_MAX))
			break;
	}

	return 0;
}
