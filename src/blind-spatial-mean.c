/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("[-g | -h | -l power | -p power | -v]")
/* TODO add [-w weight-stream] for [-ghlpv] */

/* Because the syntax for a function returning a function pointer is disgusting. */
typedef void (*process_func)(struct stream *stream);

/*
 * X-parameter 1: method enum value
 * X-parameter 2: identifier-friendly name
 * X-parameter 3: initial assignments
 * X-parameter 4: initial value
 * X-parameter 5: subcell processing
 * X-parameter 6: subcell finalisation
 */
#define LIST_MEANS(TYPE)\
	/* [default] arithmetic mean */\
	X(ARITHMETIC, arithmetic,, 0, img[j & 3] += *buf, img[j & 3] /= pixels)\
	/* geometric mean */\
	X(GEOMETRIC, geometric,, 1, img[j & 3] *= *buf, img[j & 3] = nnpow(img[j & 3], 1 / pixels))\
	/* harmonic mean */\
	X(HARMONIC, harmonic,, 0, img[j & 3] += (TYPE)1 / *buf, img[j & 3] = pixels / img[j & 3])\
	/* Lehmer mean */\
	X(LEHMER, lehmer, (a = (TYPE)power, b = a - (TYPE)1), 0,\
	  (img[j & 3] += nnpow(*buf, a), aux[j & 3] += nnpow(*buf, b)), img[j & 3] /= aux[j & 3])\
	/* power mean (Hölder mean) (m = 2 for root square mean; m = 3 for cubic mean) */\
	X(POWER, power, a = (TYPE)power, 0, img[j & 3] += nnpow(*buf, a),\
	  img[j & 3] = nnpow(img[j & 3], (TYPE)(1. / power)) / pixels)\
	/* variance */\
	X(VARIANCE, variance,, 0, (img[j & 3] += *buf * *buf, aux[j & 3] += *buf),\
	  img[j & 3] = (img[j & 3] - aux[j & 3] * aux[j & 3] / pixels) / pixels)

#define X(V, ...) V,
enum method { LIST_MEANS() };
#undef X

static double power;

#define MAKE_PROCESS(PIXFMT, TYPE,\
		     _1, NAME, INIT, INITIAL, PROCESS_SUBCELL, FINALISE_SUBCELL)\
	static void\
	process_##PIXFMT##_##NAME(struct stream *stream)\
	{\
		TYPE img[4], aux[4], *buf, a, b;\
		TYPE pixels = (TYPE)(stream->frame_size / sizeof(img));\
		size_t i, n, j = 0, m = stream->frame_size / sizeof(*img);\
		int first = 1;\
		INIT;\
		do {\
			n = stream->ptr / stream->pixel_size * stream->n_chan;\
			buf = (TYPE *)(stream->buf);\
			for (i = 0; i < n; i++, buf++, j++, j %= m) {\
				if (!j) {\
					if (!first) {\
						for (j = 0; j < ELEMENTSOF(img); j++)\
							FINALISE_SUBCELL;\
						j = 0;\
						ewriteall(STDOUT_FILENO, img, sizeof(img), "<stdout>");\
					}\
					first = 0;\
					img[0] = aux[0] = INITIAL;\
					img[1] = aux[1] = INITIAL;\
					img[2] = aux[2] = INITIAL;\
					img[3] = aux[3] = INITIAL;\
				}\
				PROCESS_SUBCELL;\
			}\
			n *= sizeof(TYPE);\
			memmove(stream->buf, stream->buf + n, stream->ptr -= n);\
		} while (eread_stream(stream, SIZE_MAX));\
		if (!first) {\
			for (j = 0; j < ELEMENTSOF(img); j++)\
				FINALISE_SUBCELL;\
			ewriteall(STDOUT_FILENO, img, sizeof(img), "<stdout>");\
		}\
		(void) aux, (void) a, (void) b, (void) pixels;\
	}
#define X(...) MAKE_PROCESS(lf, double, __VA_ARGS__)
LIST_MEANS(double)
#undef X
#define X(...) MAKE_PROCESS(f, float, __VA_ARGS__)
LIST_MEANS(float)
#undef X
#undef MAKE_PROCESS

#define X(ID, NAME, ...) [ID] = process_lf_##NAME,
static const process_func process_functions_lf[] = { LIST_MEANS() };
#undef X

#define X(ID, NAME, ...) [ID] = process_f_##NAME,
static const process_func process_functions_f[] = { LIST_MEANS() };
#undef X

int
main(int argc, char *argv[])
{
	struct stream stream;
	process_func process;
	enum method method = ARITHMETIC;

	ARGBEGIN {
	case 'g':
		method = GEOMETRIC;
		break;
	case 'h':
		method = HARMONIC;
		break;
	case 'l':
		method = LEHMER;
		power = etolf_flag('l', UARGF());
		break;
	case 'p':
		method = POWER;
		power = etolf_flag('p', UARGF());
		break;
	case 'v':
		method = VARIANCE;
		break;
	default:
		usage();
	} ARGEND;

	if (argc)
		usage();

	eopen_stream(&stream, NULL);

        if (stream.encoding == DOUBLE)
                process = process_functions_lf[method];
        else
                process = process_functions_f[method];


	if (DPRINTF_HEAD(STDOUT_FILENO, stream.frames, 1, 1, stream.pixfmt) < 0)
		eprintf("dprintf:");
	process(&stream);
	if (stream.ptr)
		eprintf("%s: incomplete frame\n", stream.file);
	return 0;
}
