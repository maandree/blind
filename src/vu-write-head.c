/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s parameters ...\n", argv0);
}

int
main(int argc, char *argv[])
{
	int i;

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	if (!argc)
		usage();

	printf("%s", argv[0]);
	for (i = 0; i < argc; i++)
		printf(" %s", argv[i]);
	printf("\n%cuivf", 0);

	efshut(stdout, "<stdout>");
	return 0;
}
