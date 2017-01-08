/* See LICENSE file for copyright and license details. */

#include <stdio.h>

#define ELEMENTSOF(ARRAY) (sizeof(ARRAY) / sizeof(*(ARRAY)))

void eprintf(const char *fmt, ...);
void enprintf(int status, const char *fmt, ...);
void weprintf(const char *fmt, ...);

int tollu(const char *s, unsigned long long int min, unsigned long long int max, unsigned long long int *out);
int tolli(const char *s, long long int min, long long int max, long long int *out);
#define DEF_STR_TO_INT(FNAME, INTTYPE, INTER_FNAME, INTER_INTTYPE)\
	static inline int\
	FNAME(const char *s, INTTYPE min, INTTYPE max, INTTYPE *out)\
	{\
		INTER_INTTYPE inter;\
		if (INTER_FNAME(s, (INTER_INTTYPE)min, (INTER_INTTYPE)max, &inter))\
			return -1;\
		*out = (INTTYPE)inter;\
		return 0;\
	}
DEF_STR_TO_INT(tolu, unsigned long int, tollu, unsigned long long int)
DEF_STR_TO_INT(tou, unsigned int, tollu, unsigned long long int)
DEF_STR_TO_INT(toli, long int, tolli, long long int)
DEF_STR_TO_INT(toi, int, tolli, long long int)
#undef DEF_STR_TO_INT
#define tozu tolu
#define tozi toli
#define toju tollu
#define toji tolli

int fshut(FILE *fp, const char *fname);
void enfshut(int status, FILE *fp, const char *fname);
void efshut(FILE *fp, const char *fname);
