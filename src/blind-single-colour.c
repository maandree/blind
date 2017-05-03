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
	struct stream stream = { .width = 0, .height = 0, .frames = 1 };
	double X, Y, Z, alpha = 1;
	size_t x, y, n;
	pixel_t buf[BUFSIZ / 4];
	ssize_t r;
	int inf = 0;
	char *arg;

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
		Y = etolf_arg("the Y value", argv[0]);
		X = Y * D65_XYZ_X;
		Z = Y * D65_XYZ_Z;
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
	while (inf || stream.frames--)
		for (y = stream.height; y--;)
			for (x = stream.width * sizeof(*buf); x;)
				for (x -= n = MIN(sizeof(buf), x); n; n -= (size_t)r)
					if ((r = write(STDOUT_FILENO, buf, n)) < 0)
						eprintf("write <stdout>:");

	return 0;
}
