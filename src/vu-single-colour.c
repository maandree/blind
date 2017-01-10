/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "stream.h"
#include "util.h"

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

typedef double pixel_t[4];

static void
usage(void)
{
	eprintf("usage: %s [-f frames | -f 'inf'] -w width -h height (X Y Z | Y) [alpha]\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stream stream;
	double X, Y, Z, alpha = 1;
	size_t x, y, n;
	pixel_t buf[1024];
	ssize_t r;
	int inf = 0;
	char *arg;

	stream.width = 0;
	stream.height = 0;
	stream.frames = 1;

	ARGBEGIN {
	case 'f':
		arg = EARGF(usage());
		if (!strcmp(arg, "inf"))
			inf = 1, stream.frames = 0;
		else if (tozu(arg, 1, SIZE_MAX, &stream.frames))
			eprintf("argument of -f must be an integer in [1, %zu] or 'inf'\n", SIZE_MAX);
		break;
	case 'w':
		if (tozu(EARGF(usage()), 1, SIZE_MAX, &stream.width))
			eprintf("argument of -w must be an integer in [1, %zu]\n", SIZE_MAX);
		break;
	case 'h':
		if (tozu(EARGF(usage()), 1, SIZE_MAX, &stream.height))
			eprintf("argument of -h must be an integer in [1, %zu]\n", SIZE_MAX);
		break;
	default:
		usage();
	} ARGEND;

	if (!stream.width || !stream.height || !argc || argc > 4)
		usage();

	if (argc < 3) {
		X = D65_XYY_X / D65_XYY_Y;
		Z = 1 / D65_XYY_Y - 1 - X;
		if (tolf(argv[1], &Y))
			eprintf("the Y value must be a floating-point value\n");
	} else {
		if (tolf(argv[0], &X))
			eprintf("the X value must be a floating-point value\n");
		if (tolf(argv[1], &Y))
			eprintf("the Y value must be a floating-point value\n");
		if (tolf(argv[2], &Z))
			eprintf("the Z value must be a floating-point value\n");
	}
	if (!(argc & 1) && tolf(argv[argc - 1], &alpha))
		eprintf("the alpha value must be a floating-point value\n");

	strcpy(stream.pixfmt, "xyza");
	fprint_stream_head(stdout, &stream);
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");

	for (x = 0; x < ELEMENTSOF(buf); x++) {
		buf[x][0] = X;
		buf[x][1] = Y;
		buf[x][2] = Z;
		buf[x][3] = alpha;
	}
	while (inf || stream.frames--) {
		for (y = stream.height; y--;) {
			for (x = stream.width; x;) {
				x -= n = ELEMENTSOF(buf) < x ? ELEMENTSOF(buf) : x;
				for (n *= sizeof(*buf); n; n -= (size_t)r) {
					r = write(STDOUT_FILENO, buf, n);
					if (r < 0)
						eprintf("write <stdout>:");
				}
			}
		}
	}

	return 0;
}
