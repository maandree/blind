/* See LICENSE file for copyright and license details. */
#include <math.h>

#define D65_XYY_X 0.312726871026564878786047074755
#define D65_XYY_Y 0.329023206641284038376227272238

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
	*x = MULTIPLY(0.412457445582367576708548995157, 0.357575865245515878143578447634, 0.180437247826399665973085006954);
	*y = MULTIPLY(0.212673370378408277403536885686, 0.715151730491031756287156895269, 0.072174899130559869164791564344);
	*z = MULTIPLY(0.019333942761673460208893260415, 0.119191955081838593666354597644, 0.950302838552371742508739771438);
#undef MULTIPLY
}
