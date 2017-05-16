/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("[-g | -h | -l power | -p power]")
/* TODO add -w weight-stream */

/* Because the syntax for a function returning a function pointer is disgusting. */
typedef void (*process_func)(struct stream *stream, void *buffer, void *image, size_t frame);

/*
 * X-parameter 1: method enum value
 * X-parameter 2: identifier-friendly name
 * X-parameter 3: images
 * X-parameter 4: action for first frame
 * X-parameter 5: pre-process assignments
 * X-parameter 6: subcell processing
 * X-parameter 7: pre-finalise assignments
 * X-parameter 8: subcell finalisation
 */
#define LIST_MEANS(TYPE)\
	/* [default] arithmetic mean */\
	X(ARITHMETIC, arithmetic, 1, COPY_FRAME,, *img1 += *buf,\
	  a = (TYPE)1.0 / (TYPE)frame, *img1 *= a)\
	/* geometric mean */\
	X(GEOMETRIC, geometric, 1, COPY_FRAME,, *img1 *= *buf,\
	  a = (TYPE)1.0 / (TYPE)frame, *img1 = nnpow(*img1, a))\
	/* harmonic mean */\
	X(HARMONIC, harmonic, 1, ZERO_AND_PROCESS_FRAME,, *img1 += (TYPE)1 / *buf,\
	  a = (TYPE)frame, *img1 = a / *img1)\
	/* lehmer mean */\
	X(LEHMER, lehmer, 2, ZERO_AND_PROCESS_FRAME, (a = (TYPE)power, b = a - (TYPE)1),\
	  (*img1 += nnpow(*buf, a), *img2 += nnpow(*buf, b)),, *img1 /= *img2)\
	/* power mean (Hölder mean) (m = 2 for root square mean; m = 3 for cubic mean) */\
	X(POWER, power, 1, ZERO_AND_PROCESS_FRAME, a = (TYPE)power,\
	  *img1 += nnpow(*buf, a), (a = (TYPE)1 / (TYPE)frame, b = (TYPE)(1.0 / power)),\
	  *img1 = a * nnpow(*img1, b))

enum first_frame_action {
	COPY_FRAME,
	PROCESS_FRAME,
	ZERO_AND_PROCESS_FRAME,
};

#define X(V, ...) V,
enum method { LIST_MEANS() };
#undef X

static double power;

#define MAKE_PROCESS(PIXFMT, TYPE,\
		     _1, NAME, _3, _4, PRE_PROCESS, PROCESS_SUBCELL, PRE_FINALISE, FINALISE_SUBCELL)\
	static void\
	process_##PIXFMT##_##NAME(struct stream *stream, void *buffer, void *image, size_t frame)\
	{\
		TYPE *buf = buffer, *img1 = image, a, b;\
		TYPE *img2 = (TYPE *)(((char *)image) + stream->frame_size);\
		size_t x, y;\
		if (!stream) {\
			PRE_FINALISE;\
			for (y = 0; y < stream->height; y++)\
				for (x = 0; x < stream->width; x++, img1++, img2++, buf++)\
					FINALISE_SUBCELL;\
		} else {\
			PRE_PROCESS;\
			for (y = 0; y < stream->height; y++)\
				for (x = 0; x < stream->width; x++, img1++, img2++, buf++)\
					PROCESS_SUBCELL;\
		}\
		(void) img2, (void) a, (void) b, (void) frame;\
	}
#define X(...) MAKE_PROCESS(xyza, double, __VA_ARGS__)
LIST_MEANS(double)
#undef X
#define X(...) MAKE_PROCESS(xyzaf, float, __VA_ARGS__)
LIST_MEANS(float)
#undef X
#undef MAKE_PROCESS

#define X(ID, NAME, ...) [ID] = process_xyza_##NAME,
static const process_func process_functions_xyza[] = { LIST_MEANS() };
#undef X

#define X(ID, NAME, ...) [ID] = process_xyzaf_##NAME,
static const process_func process_functions_xyzaf[] = { LIST_MEANS() };
#undef X

int
main(int argc, char *argv[])
{
	struct stream stream;
	void *buf, *img;
	process_func process;
	size_t frames, images;
	enum method method = ARITHMETIC;
	enum first_frame_action first_frame_action;

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
	default:
		usage();
	} ARGEND;

	if (argc)
		usage();

#define X(ID, _2, IMAGES, FIRST_FRAME_ACTION, ...)\
	case ID:\
		images = IMAGES;\
		first_frame_action = FIRST_FRAME_ACTION;\
		break;
	switch (method) {
	LIST_MEANS()
	default:
		abort();
	}
#undef X

	eopen_stream(&stream, NULL);

        if (!strcmp(stream.pixfmt, "xyza"))
                process = process_functions_xyza[method];
        else if (!strcmp(stream.pixfmt, "xyza f"))
                process = process_functions_xyzaf[method];
        else
                eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	stream.frames = 1;
	echeck_dimensions(&stream, WIDTH | HEIGHT, NULL);
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	buf = emalloc(stream.frame_size);
	if (first_frame_action == ZERO_AND_PROCESS_FRAME)
		img = ecalloc(images, stream.frame_size);
	else
		img = emalloc2(images, stream.frame_size);

	frames = 0;
	if (first_frame_action == COPY_FRAME) {
		if (!eread_frame(&stream, buf))
			eprintf("video is no frames\n");
		frames++;
	}
	for (; eread_frame(&stream, buf); frames++)
		process(&stream, buf, img, frames);
	if (!frames)
		eprintf("video is no frames\n");
	process(&stream, NULL, img, frames);

	ewriteall(STDOUT_FILENO, img, stream.frame_size, "<stdout>");
	free(buf);
	free(img);
	return 0;
}
