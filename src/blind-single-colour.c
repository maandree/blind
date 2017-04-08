/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

USAGE("[-f frames | -f 'inf'] -w width -h height (X Y Z | Y) [alpha]")

typedef double pixel_t[4];

int
main(int argc, char *argv[])
{
	struct stream stream;
	double X, Y, Z, alpha = 1;
	size_t x, y, n;
	pixel_t buf[BUFSIZ / 4];
	ssize_t r;
	int inf = 0;
	char *arg;

	stream.width = 0;
	stream.height = 0;
	stream.frames = 1;

	ARGBEGIN {
	case 'f':
		arg = UARGF();
		if (!strcmp(arg, "inf"))
			inf = 1, stream.frames = 0;
		else
			stream.frames = etozu_flag('f', arg, 1, SIZE_MAX);
		break;
	case 'w':
		stream.width = etozu_flag('w', UARGF(), 1, SIZE_MAX);
		break;
	case 'h':
		stream.height = etozu_flag('h', UARGF(), 1, SIZE_MAX);
		break;
	default:
		usage();
	} ARGEND;

	if (!stream.width || !stream.height || !argc || argc > 4)
		usage();

	if (argc < 3) {
		X = D65_XYY_X / D65_XYY_Y;
		Z = 1 / D65_XYY_Y - 1 - X;
		Y = etolf_arg("the Y value", argv[0]);
		X *= Y;
		Z *= Y;
	} else {
		X = etolf_arg("the X value", argv[0]);
		Y = etolf_arg("the Y value", argv[1]);
		Z = etolf_arg("the Z value", argv[2]);
	}
	if (~argc & 1)
		alpha = etolf_arg("the alpha value", argv[argc - 1]);

	if (inf)
		einf_check_fd(STDOUT_FILENO, "<stdout>");

	strcpy(stream.pixfmt, "xyza");
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");

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
