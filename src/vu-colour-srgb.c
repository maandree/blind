/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-d depth] [-l] red green blue\n", argv0);
}

int
main(int argc, char *argv[])
{
	unsigned long long int max;
	double red, green, blue, X, Y, Z;
	int depth = 8;
	int linear = 0;

	ARGBEGIN {
	case 'd':
		if (toi(EARGF(usage()), 1, 64, &depth))
			eprintf("argument of -d must be an integer in [1, 64]\n");
		break;
	case 'l':
		linear = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 3)
		usage();

	if (tolf(argv[0], &red))
		eprintf("the X value must be a floating-point value\n");
	if (tolf(argv[1], &green))
		eprintf("the Y value must be a floating-point value\n");
	if (tolf(argv[2], &blue))
		eprintf("the Z value must be a floating-point value\n");

	max    = 1ULL << (depth - 1);
	max   |= max - 1;
	red   /= max;
	green /= max;
	blue  /= max;
	if (!linear) {
		red   = srgb_decode(red);
		green = srgb_decode(green);
		blue  = srgb_decode(blue);
	}

	srgb_to_ciexyz(red, green, blue, &X, &Y, &Z);
	printf("%lf %lf %lf\n", X, Y, Z);
	efshut(stdout, "<stdout>");

	return 0;
}
