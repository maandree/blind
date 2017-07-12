/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("(X Y Z | Y)")

int
main(int argc, char *argv[])
{
	double X, Y, Z;

	UNOFLAGS(0);

	if (argc == 1) {
		Y = etolf_arg("the Y value", argv[0]);
		X = Y * D65_XYZ_X;
		Z = Y * D65_XYZ_Z;
		printf("%lf %lf %lf\n", X, Y, Z);
	} else if (argc == 3) {
		printf("%s %s %s\n", argv[0], argv[1], argv[2]);
	} else {
		usage();
	}

	efshut(stdout, "<stdout>");
	return 0;
}
