/* See LICENSE file for copyright and license details. */
#include "util.h"

USAGE("(X Y Z | Y)")

int
main(int argc, char *argv[])
{
	UNOFLAGS(0);

	if (argc == 1)
		printf("%s\n", argv[0]);
	else if (argc == 3)
		printf("%s %s %s\n", argv[0], argv[1], argv[2]);
	else
		usage();

	efshut(stdout, "<stdout>");
	return 0;
}
