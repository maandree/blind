/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("[-s]")

#define CONV(ITYPE, SITYPE, OTYPE, EXPONENT, HA2EXPONENT, FRACTION)\
	do {\
		static int cache_i = 0;\
		static ITYPE cache_in[] = {0, 0, 0, 0};\
		static OTYPE cache_out[] = {0, 0, 0, 0};\
		ITYPE exponent, fraction, signb;\
		SITYPE sexponent;\
		OTYPE ret, dexponent;\
		if (portable == cache_in[cache_i]) {\
			ret = cache_out[cache_i++];\
			cache_i &= 3;\
			return ret;\
		}\
		cache_in[cache_i] = portable;\
		signb = portable >> (EXPONENT + FRACTION);\
		exponent = (portable >> FRACTION) ^ (signb << EXPONENT);\
		fraction = portable & (((ITYPE)1 << FRACTION) - 1);\
		if (!exponent) {\
			if (!fraction) {\
				ret = (OTYPE)0.0;\
			} else {\
				sexponent = 1 - HA2EXPONENT - FRACTION;\
				dexponent = (OTYPE)sexponent;\
				ret = (OTYPE)fraction;\
				ret *= pow((OTYPE)2.0, dexponent);\
			}\
		} else if (exponent + 1 == (ITYPE)1 << EXPONENT) {\
			ret = (OTYPE)(fraction ? NAN : INFINITY);\
		} else {\
			fraction |= (ITYPE)1 << FRACTION;\
			sexponent = (SITYPE)exponent;\
			sexponent -= HA2EXPONENT + FRACTION;\
			dexponent = (OTYPE)sexponent;\
			ret = (OTYPE)fraction;\
			ret *= pow((OTYPE)2.0, dexponent);\
		}\
		ret = signb ? -ret : ret;\
		cache_out[cache_i++] = ret;\
		cache_i &= 3;\
		return ret;\
	} while (0)

#define PROCESS(ITYPE, OTYPE, BITS)\
	do {\
		size_t i, n;\
		ITYPE *ibuf = (ITYPE *)(stream->buf);\
		OTYPE *obuf = sizeof(ITYPE) == sizeof(OTYPE) ? (OTYPE *)(stream->buf)\
			: alloca(sizeof(stream->buf) / sizeof(ITYPE) * sizeof(OTYPE));\
		strict *= !USING_BINARY##BITS;\
		if (!strict && sizeof(ITYPE) != sizeof(OTYPE))\
			eprintf("-s is required on this machine\n");\
		do {\
			n = stream->ptr / sizeof(ITYPE);\
			if (strict) {\
				for (i = 0; i < n; i++)\
					obuf[i] = conv_##OTYPE(le##BITS##toh(ibuf[i]));\
			} else {\
				for (i = 0; i < n; i++)\
					obuf[i] = *(OTYPE *)&(ITYPE){le##BITS##toh(ibuf[i])};\
			}\
			ewriteall(STDOUT_FILENO, obuf, n * sizeof(OTYPE), "<stdout>");\
			n *= sizeof(ITYPE);\
			memmove(stream->buf, stream->buf + n, stream->ptr -= n);\
		} while (eread_stream(stream, SIZE_MAX));\
		if (stream->ptr)\
			eprintf("%s: incomplete frame\n", stream->file);\
	} while (0)

static double conv_double(uint64_t portable) {CONV(uint64_t, int64_t, double, 11, 1023, 52);}
static float  conv_float (uint32_t portable) {CONV(uint32_t, int32_t, float, 8, 127, 23);}

static void process_xyza (struct stream *stream, int strict) {PROCESS(uint64_t, double, 64);}
static void process_xyzaf(struct stream *stream, int strict) {PROCESS(uint32_t, float, 32);}

int
main(int argc, char *argv[])
{
	struct stream stream;
	int strict = 1;
	void (*process)(struct stream *stream, int strict);

	ARGBEGIN {
	case 's':
		strict = 0;
		break;
	default:
		usage();
	} ARGEND;
	if (argc)
		usage();

	eopen_stream(&stream, NULL);

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_xyza;
	else if (!strcmp(stream.pixfmt, "xyza f"))
		process = process_xyzaf;
	else
		eprintf("pixel format %s is not supported\n", stream.pixfmt);

	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	process(&stream, strict);
	return 0;
}
