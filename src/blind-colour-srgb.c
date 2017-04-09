/* See LICENSE file for copyright and license details. */
#include "util.h"

USAGE("[-d depth] [-l] red green blue")

int
main(int argc, char *argv[])
{
	unsigned long long int max;
	double red, green, blue, X, Y, Z;
	int depth = 8, linear = 0;

	ARGBEGIN {
	case 'd':
		depth = etoi_flag('d', UARGF(), 1, 64);
		break;
	case 'l':
		linear = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 3)
		usage();

	max   = 1ULL << (depth - 1);
	max  |= max - 1;
	red   = etolf_arg("the red value",   argv[0]) / max;
	green = etolf_arg("the green value", argv[1]) / max;
	blue  = etolf_arg("the blue value",  argv[2]) / max;
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
