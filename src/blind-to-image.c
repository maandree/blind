/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

USAGE("[-d depth | -f]")

static int luma_warning_triggered = 0;
static int gamut_warning_triggered = 0;
static int alpha_warning_triggered = 0;
static unsigned long long int max;
static int bytes;

static void
write_pixel(double R, double G, double B, double A)
{
	unsigned long long int colours[4];
	unsigned char buf[4 * 8];
	int i, j, k, bm = bytes - 1;

	if (R < 0 || G < 0 || B < 0 || R > 1 || G > 1 || B > 1) {
		if (gamut_warning_triggered) {
			gamut_warning_triggered = 1;
			weprintf("warning: out-of-gamut colour detected\n");
		}
		; /* TODO gamut */
		R = CLIP(0, R, 1);
		G = CLIP(0, G, 1);
		B = CLIP(0, B, 1);
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

	for (i = k = 0; i < 4; i++, k += bytes) {
		for (j = 0; j < bytes; j++) {
			buf[k + bm - j] = (unsigned char)(colours[i]);
			colours[i] >>= 8;
		}
	}

	ewriteall(STDOUT_FILENO, buf, k, "<stdout>");
}

static void
process_xyza(struct stream *stream, size_t n)
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

		ciexyz_to_srgb(X, Y, Z, &R, &G, &B);
		write_pixel(R, G, B, A);
	}
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	int depth = 16, farbfeld = 0;
	void (*process)(struct stream *stream, size_t n);

	ARGBEGIN {
	case 'd':
		depth = etoi_flag('d', UARGF(), 1, 64);
		break;
	case 'f':
		farbfeld = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc || (farbfeld && depth != 16))
		usage();

	eopen_stream(&stream, NULL);

	max = 1ULL << (depth - 1);
	max |= max - 1;
	bytes = (depth + 7) / 8;

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	if (farbfeld) {
		uint32_t width = stream.width, height = stream.height;
		if (stream.width > UINT32_MAX)
			eprintf("%s: frame is too wide\n", stream.file);
		if (stream.height > UINT32_MAX)
			eprintf("%s: frame is too tall\n", stream.file);
		printf("farbfeld");
		memmove(stream.buf + 8, stream.buf, stream.ptr);
		stream.ptr += 8;
		width = htonl(width);
		height = htonl(height);
		memcpy(stream.buf + 0, &width, 4);
		memcpy(stream.buf + 4, &height, 4);
	} else {
		printf("P7\n"
		       "WIDTH %zu\n"
		       "HEIGHT %zu\n"
		       "DEPTH 4\n" /* channels */
		       "MAXVAL %llu\n"
		       "TUPLTYPE RGB_ALPHA\n"
		       "ENDHDR\n", stream.width, stream.height, max);
	}
	efflush(stdout, "<stdout>");

	process_stream(&stream, process);
	return 0;
}
