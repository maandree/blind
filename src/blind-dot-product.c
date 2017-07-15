/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("right-hand-stream")

#define PROCESS(TYPE, SUFFIX)\
	static void\
	process_##SUFFIX(struct stream *left, struct stream *right, size_t n)\
	{\
		size_t i, j, s = left->n_chan * sizeof(TYPE);\
		TYPE v, *l, *r;\
		for (i = 0; i < n; i += s) {\
			l = (TYPE *)(left->buf + i);\
			r = (TYPE *)(right->buf + i);\
			v = 0;\
			for (j = 0; j < left->n_chan; j++)\
				v += l[j] * r[j];\
			for (j = 0; j < left->n_chan; j++)\
				l[j] = v;\
		}\
	}

PROCESS(double, lf)
PROCESS(float, f)

int
main(int argc, char *argv[])
{
	struct stream left, right;
	void (*process)(struct stream *left, struct stream *right, size_t n);

	UNOFLAGS(argc != 1);

	eopen_stream(&left, NULL);
	eopen_stream(&right, argv[0]);

	if (left.encoding == DOUBLE)
		process = process_lf;
	else if (left.encoding == FLOAT)
		process = process_f;
	else
		eprintf("pixel format %s is not supported, try xyza\n", left.pixfmt);

	fprint_stream_head(stdout, &left);
	efflush(stdout, "<stdout>");
	process_two_streams(&left, &right, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
