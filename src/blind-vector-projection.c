/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("[-r | -s] plane-stream")

static int level = 1;

#define PROCESS(TYPE, SUFFIX)\
	static void\
	process_##SUFFIX(struct stream *left, struct stream *right, size_t n)\
	{\
		size_t i;\
		TYPE *lx, *ly, *lz, *la, rx, ry, rz, ra, x, y, z, a, norm;\
		for (i = 0; i < n; i += 4 * sizeof(TYPE)) {\
			lx = ((TYPE *)(left->buf + i)) + 0, rx = ((TYPE *)(right->buf + i))[0];\
			ly = ((TYPE *)(left->buf + i)) + 1, ry = ((TYPE *)(right->buf + i))[1];\
			lz = ((TYPE *)(left->buf + i)) + 2, rz = ((TYPE *)(right->buf + i))[2];\
			la = ((TYPE *)(left->buf + i)) + 3, ra = ((TYPE *)(right->buf + i))[3];\
			norm = rx * rx + ry * ry + rz * rz + ra * ra;\
			norm = sqrt(norm);\
			x = y = z = a = *lx * rx + *ly * ry + *lz * rz + *la * ra;\
			if (level) {\
				x *= rx;\
				y *= ry;\
				z *= rz;\
				a *= rz;\
				if (level > 1) {\
					x = *lx - x;\
					y = *ly - y;\
					z = *lz - z;\
					a = *la - a;\
				}\
			}\
			*lx = x;\
			*ly = y;\
			*lz = z;\
			*la = a;\
		}\
	}

PROCESS(double, lf)
PROCESS(float, f)

int
main(int argc, char *argv[])
{
	struct stream left, right;
	void (*process)(struct stream *left, struct stream *right, size_t n);

	ARGBEGIN {
	case 'r':
		if (level == 0)
			usage();
		level = 2;
		break;
	case 's':
		if (level == 2)
			usage();
		level = 0;
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 1)
		usage();

	eopen_stream(&left, NULL);
	eopen_stream(&right, argv[0]);

	if (!strcmp(left.pixfmt, "xyza"))
		process = process_lf;
	else if (!strcmp(left.pixfmt, "xyza f"))
		process = process_f;
	else
		eprintf("pixel format %s is not supported, try xyza\n", left.pixfmt);

	fprint_stream_head(stdout, &left);
	efflush(stdout, "<stdout>");
	process_two_streams(&left, &right, STDOUT_FILENO, "<stdout>", process);
	return 0;
}
