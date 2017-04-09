/* See LICENSE file for copyright and license details. */
#include "util.h"

USAGE("parameters ...")

int
main(int argc, char *argv[])
{
	UNOFLAGS(!argc);

	printf("%s", *argv++);
	while (*argv)
		printf(" %s", *argv++);
	printf("\n%cuivf", 0);

	efshut(stdout, "<stdout>");
	return 0;
}
