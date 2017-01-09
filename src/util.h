/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define D65_XYY_X 0.312726871026564878786047074755
#define D65_XYY_Y 0.329023206641284038376227272238

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

static inline int
tolf(const char *s, double *out)
{
	char *end;
	errno = 0;
	*out = strtod(s, &end);
	if (errno) {
		return -1;
	} else if (*end) {
		errno = EINVAL;
		return -1;
	}
	return 0;
}

int fshut(FILE *fp, const char *fname);
void enfshut(int status, FILE *fp, const char *fname);
void efshut(FILE *fp, const char *fname);

static inline double
srgb_encode(double t)
{
	double sign = 1;
	if (t < 0) {
		t = -t;
		sign = -1;
	}
	t = t <= 0.0031306684425217108 ? 12.92 * t : 1.055 * pow(t, 1 / 2.4) - 0.055;
	return t * sign;
}

static inline double
srgb_decode(double t)
{
	double sign = 1;
	if (t < 0) {
		t = -t;
		sign = -1;
	}
	t = t <= 0.0031306684425217108 * 12.92 ? t / 12.92 : pow((t + 0.055) / 1.055, 2.4);
	return t * sign;
}

static inline void
ciexyz_to_srgb(double x, double y, double z, double *r, double *g, double *b)
{
#define MULTIPLY(CX, CY, CZ)  ((CX) * x + (CY) * y + (CZ) * z)
	*r = MULTIPLY(3.240446254647737500675930277794, -1.537134761820080575134284117667, -0.498530193022728718155178739835);
	*g = MULTIPLY(-0.969266606244679751469561779231, 1.876011959788370209167851498933, 0.041556042214430065351304932619);
	*b = MULTIPLY(0.055643503564352832235773149705, -0.204026179735960239147729566866, 1.057226567722703292062647051353);
#undef MULTIPLY
}

static inline void
srgb_to_ciexyz(double r, double g, double b, double *x, double *y, double *z)
{
#define MULTIPLY(CR, CG, CB) ((CR) * r + (CG) * g + (CB) * b)
	*x = MULTIPLIY(0.412457445582367576708548995157, 0.357575865245515878143578447634, 0.180437247826399665973085006954);
	*y = MULTIPLIY(0.212673370378408277403536885686, 0.715151730491031756287156895269, 0.072174899130559869164791564344);
	*z = MULTIPLIY(0.019333942761673460208893260415, 0.119191955081838593666354597644, 0.950302838552371742508739771438);
#undef MULTIPLY

}
