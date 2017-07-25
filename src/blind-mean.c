/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("[-g | -h | -H | -i | -l power | -L | -p power | -s power | -v | -z power] stream-1 stream-2 ...")
/* TODO add [-w weight-stream] for [-ghlpv] */

/* Because the syntax for a function returning a function pointer is disgusting. */
typedef void (*process_func)(struct stream *streams, size_t n_streams, size_t n);

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
	X(ARITHMETIC, arithmetic, sn = (TYPE)1 / sn, 0, img += val, img *= sn) \
	/* geometric mean */\
	X(GEOMETRIC, geometric, sn = (TYPE)1 / sn, 1, img *= val, img = nnpow(img, sn))\
	/* harmonic mean */\
	X(HARMONIC, harmonic,, 0, img += (TYPE)1 / val, img = sn / img)\
	/* Heronian mean */\
	X(HERONIAN, heronian,, 0, auxs[j] = val,\
	  img = (auxs[0] + sqrt(auxs[0] * auxs[1]) + auxs[1]) / (TYPE)3)\
	/* identric mean */\
	X(IDENTRIC, identric, a = (TYPE)(1. / M_E), 0, auxs[j] = val,\
	  img = auxs[0] == auxs[1] ? auxs[0] :\
	        nnpow(nnpow(auxs[0], auxs[0]) / nnpow(auxs[1], auxs[1]), auxs[0] - auxs[1]) * a)\
	/* Lehmer mean */\
	X(LEHMER, lehmer, (a = (TYPE)power, b = a - (TYPE)1), 0,\
	  (img += nnpow(val, a), aux += nnpow(val, b)), img /= aux)\
	/* logarithmic mean */\
	X(LOGARITHMIC, logarithmic,, 0, auxs[j] = val,\
	  img = auxs[0] == auxs[1] ? auxs[0] : (!auxs[0] || !auxs[1]) ? (TYPE)0 :\
	        (auxs[1] - auxs[0]) / log(auxs[1] / auxs[0]))\
	/* power mean (Hölder mean) (m = 2 for root square mean; m = 3 for cubic mean) */\
	X(POWER, power, (a = (TYPE)power, b = (TYPE)(1. / power), sn = (TYPE)1 / sn), 0,\
	  img += nnpow(val, a), img = nnpow(img, b) * sn)\
	/* Stolarsky mean */\
	X(STOLARSKY, stolarsky, (a = (TYPE)power, b = (TYPE)(1. / (power - 1.))), 0, auxs[j] = val,\
	  img = auxs[0] == auxs[1] ? auxs[0] :\
	        nnpow((nnpow(auxs[0], auxs[0]) - nnpow(auxs[1], auxs[1])) /\
		      (a * (auxs[0] - auxs[1])), b))\
	/* variance */\
	X(VARIANCE, variance, sn = (TYPE)1 / sn, 0, (img += val * val, aux += val),\
	  img = (img - aux * aux * sn) * sn)\
	/* Heinz mean */\
	X(HEINZ, heinz, (a = (TYPE)power, b = (TYPE)1 - a), 0, auxs[j] = val,\
	  img = (nnpow(auxs[0], a) * nnpow(auxs[1], b) + nnpow(auxs[0], b) * nnpow(auxs[1], 0)) / (TYPE)2)

#define X(V, ...) V,
enum method { LIST_MEANS() };
#undef X

static double power;

#define aux (*auxs)
#define MAKE_PROCESS(PIXFMT, TYPE,\
	  _1, NAME, INIT, INITIAL, PROCESS_SUBCELL, FINALISE_SUBCELL)\
	static void\
	process_##PIXFMT##_##NAME(struct stream *streams, size_t n_streams, size_t n)\
	{\
		size_t i, j;\
		TYPE img, auxs[2], val, a, b, sn = (TYPE)n_streams;\
		INIT;\
		for (i = 0; i < n; i += sizeof(TYPE)) {\
			img = auxs[0] = auxs[1] = INITIAL;\
			for (j = 0; j < n_streams; j++) {\
				val = *(TYPE *)(streams[j].buf + i);\
				PROCESS_SUBCELL;\
			}\
			FINALISE_SUBCELL;\
			*(TYPE *)(streams->buf + i) = img;\
		}\
		(void) aux, (void) a, (void) b, (void) sn;\
	}
#define X(...) MAKE_PROCESS(lf, double, __VA_ARGS__)
LIST_MEANS(double)
#undef X
#define X(...) MAKE_PROCESS(f, float, __VA_ARGS__)
LIST_MEANS(float)
#undef X
#undef MAKE_PROCESS
#undef aux

#define X(ID, NAME, ...) [ID] = process_lf_##NAME,
static const process_func process_functions_lf[] = { LIST_MEANS() };
#undef X

#define X(ID, NAME, ...) [ID] = process_f_##NAME,
static const process_func process_functions_f[] = { LIST_MEANS() };
#undef X

int
main(int argc, char *argv[])
{
	struct stream *streams;
	process_func process;
	size_t frames = SIZE_MAX, tmp;
	enum method method = ARITHMETIC;
	int i, two = 0;

	ARGBEGIN {
	case 'g':
		method = GEOMETRIC;
		break;
	case 'h':
		method = HARMONIC;
		break;
	case 'H':
		method = HERONIAN;
		two = 1;
		break;
	case 'i':
		method = IDENTRIC;
		two = 1;
		break;
	case 'l':
		method = LEHMER;
		power = etolf_flag('l', UARGF());
		break;
	case 'L':
		method = LOGARITHMIC;
		two = 1;
		break;
	case 'p':
		method = POWER;
		power = etolf_flag('p', UARGF());
		break;
	case 's':
		method = STOLARSKY;
		two = 1;
		power = etolf_flag('s', UARGF());
		break;
	case 'v':
		method = VARIANCE;
		break;
	case 'z':
		method = HEINZ;
		two = 1;
		power = etolf_flag('z', UARGF());
		break;
	default:
		usage();
	} ARGEND;

	if (argc < 2 || (argc > 2 && two))
		usage();

	streams = alloca((size_t)argc * sizeof(*streams));
	for (i = 0; i < argc; i++) {
		eopen_stream(streams + i, argv[i]);
		if (streams[i].frames && streams[i].frames < frames)
			frames = streams[i].frames;
	}

        if (streams->encoding == DOUBLE)
                process = process_functions_lf[method];
        else
                process = process_functions_f[method];

	tmp = streams->frames, streams->frames = frames;
	fprint_stream_head(stdout, streams);
	efflush(stdout, "<stdout>");
	streams->frames = tmp;
	process_multiple_streams(streams, (size_t)argc, STDOUT_FILENO, "<stdout>", 1, process);
	return 0;
}
