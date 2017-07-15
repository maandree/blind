#include <stdint.h>
#include <stdio.h>

int
main(void)
{
	if (sizeof(float) == 4) {
		unsigned long int a, b;
		a = (unsigned long int)*(uint32_t *)&(float){  (float)(1. / 12.) };
		b = (unsigned long int)*(uint32_t *)&(float){ -(float)(1. / 12.) };
		printf("#define USING_BINARY32 %i\n",
		       a == 0x3daaaaabUL && b == 0xbdaaaaabUL);
	}
	if (sizeof(double) == 8) {
		unsigned long long int a, b;
		a = (unsigned long long int)*(uint64_t *)&(double){  1. / 12. };
		b = (unsigned long long int)*(uint64_t *)&(double){ -1. / 12. };
		printf("#define USING_BINARY64 %i\n",
		       a == 0x3fb5555555555555ULL && b == 0xbfb5555555555555ULL);
	}
	return 0;
}
