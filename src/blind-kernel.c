/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <math.h>
#include <string.h>

USAGE("[-xyza] kernel [parameter] ...")

#define SUBUSAGE(FORMAT)          "Usage: %s [-xyza] " FORMAT, argv0
#define MATRIX(...)               ((const double[]){__VA_ARGS__});
#define STRCASEEQ3(A, B1, B2, B3) (!strcasecmp(A, B1) || !strcasecmp(A, B2) || !strcasecmp(A, B3))

#define LIST_KERNELS\
	X(kernel_kirsch,   "kirsch")\
	X(kernel_box_blur, "box blur")\
	X(kernel_sharpen,  "sharpen")\
	X(kernel_gaussian, "gaussian")

static const double *
kernel_kirsch(int argc, char *argv[], size_t *rows, size_t *cols, double **free_this)
{
	*free_this = NULL;
	*rows = *cols = 3;
	if (argc != 1)
		eprintf(SUBUSAGE("'kirsch' direction"));
	if (STRCASEEQ3(argv[0], "1", "N",  "N"))  return MATRIX( 5,  5,  5,   -3, 0, -3,   -3, -3, -3);
	if (STRCASEEQ3(argv[0], "2", "NW", "WN")) return MATRIX( 5,  5, -3,    5, 0, -3,   -3, -3, -3);
	if (STRCASEEQ3(argv[0], "3", "W",  "W"))  return MATRIX( 5, -3, -3,    5, 0, -3,    5, -3, -3);
	if (STRCASEEQ3(argv[0], "4", "SW", "WS")) return MATRIX(-3, -3, -3,    5, 0, -3,    5,  5, -3);
	if (STRCASEEQ3(argv[0], "5", "S",  "S"))  return MATRIX(-3, -3, -3,   -3, 0, -3,    5,  5,  5);
	if (STRCASEEQ3(argv[0], "6", "SE", "ES")) return MATRIX(-3, -3, -3,   -3, 0,  5,   -3,  5,  5);
	if (STRCASEEQ3(argv[0], "7", "E",  "E"))  return MATRIX(-3, -3,  5,   -3, 0,  5,   -3, -3,  5);
	if (STRCASEEQ3(argv[0], "8", "NE", "EN")) return MATRIX(-3,  5,  5,   -3, 0,  5,   -3, -3, -3);
	eprintf("Unrecognised direction: %s\n", argv[0]);
	return NULL;
}

static const double *
kernel_box_blur(int argc, char *argv[], size_t *rows, size_t *cols, double **free_this)
{
	size_t sx, sy, i, n;
	double *cells, value;
	*free_this = NULL;
	*rows = *cols = 3;
	if (argc > 3)
		eprintf(SUBUSAGE("'box blur' [spread | x-spread y-spread]"));
	if (!argc)
		return MATRIX(1.0 / 9.0,  1.0 / 9.0,  1.0 / 9.0,
			      1.0 / 9.0,  1.0 / 9.0,  1.0 / 9.0,
			      1.0 / 9.0,  1.0 / 9.0,  1.0 / 9.0);
	if (argc == 1) {
		sx = sy = etozu_arg("spread", argv[0], 0, SIZE_MAX / 2);
	} else {
		sx = etozu_arg("x-spread", argv[0], 0, SIZE_MAX / 2);
		sy = etozu_arg("y-spread", argv[1], 0, SIZE_MAX / 2);
	}
	*rows = 2 * sy + 1;
	*cols = 2 * sx + 1;
	*free_this = cells = emalloc3(*rows, *cols, sizeof(double));
	n = (2 * sy + 1) * (2 * sx + 1);
	value = 1 / (double)n;
	for (i = 0; i < n; i++)
		cells[i] = value;
	return cells;
}

static const double *
kernel_sharpen(int argc, char *argv[], size_t *rows, size_t *cols, double **free_this)
{
	*free_this = NULL;
	*rows = *cols = 3;
	if (argc)
		eprintf(SUBUSAGE("'sharpen'"));
	return MATRIX(0, -1, 0,  -1, 5, -1,  0, -1, -0);
	(void) argv;
}

static const double *
kernel_gaussian(int argc, char *argv[], size_t *rows, size_t *cols, double **free_this)
{
	size_t spread = 0, y, x;
	int unsharpen = 0;
	double sigma, value, c, k;
	char *arg;

#define argv0 arg
	argc++, argv--;
	ARGBEGIN {
	case 's':
		if (!(arg = ARGF()))
			goto usage;
		spread = etozu_flag('s', arg, 1, SIZE_MAX / 2);
		break;
	case 'u':
		unsharpen = 1;
		break;
	default:
		goto usage;
	} ARGEND;
#undef argv0

	if (argc != 1)
		goto usage;

	sigma = etof_arg("standard-deviation", argv[0]);

	if (!spread)
		spread = (size_t)(sigma * 3.0 + 0.5);
	*rows = *cols = spread * 2 + 1;

	*free_this = emalloc3(*rows, *cols, sizeof(double));

	k = sigma * sigma * 2;
	c = M_PI * k;
	c = sqrt(c);
	c = 1.0 / c;
	k = 1.0 / -k;

	for (x = 0; x <= spread; x++) {
		value = spread - x;
		value *= value * k;
		value = exp(value) * c;
		for (y = 0; y < *rows; y++) {
			(*free_this)[y * *cols + x] = value;
			(*free_this)[y + 1 * *cols + *cols - 1 - x] = value;
		}
	}

	for (y = 0; y <= spread; y++) {
		value = spread - y;
		value *= value * k;
		value = exp(value) * c;
		for (x = 0; x < *cols; x++) {
			(*free_this)[y * *cols + x] *= value;
			(*free_this)[y + 1 * *cols + *cols - 1 - x] *= value;
		}
	}

	if (unsharpen)
		(*free_this)[spread * *cols + spread] -= 2.0;

	return *free_this;

usage:
	eprintf(SUBUSAGE("'gaussian' [-s spread] [-u] standard-deviation"));
	return NULL;
}

int
main(int argc, char *argv[])
{
	int null_x = 1, null_y = 1, null_z = 1, null_a = 1;
	size_t rows, cols, y, x, n;
	const double *kernel, *kern;
	double *buffer, *buf, *free_this;

	ARGBEGIN {
	case 'x':
		null_x = 0;
		break;
	case 'y':
		null_y = 0;
		break;
	case 'z':
		null_z = 0;
		break;
	case 'a':
		null_a = 0;
		break;
	default:
		usage();
	} ARGEND;

	if (null_x && null_y && null_z && null_a)
		null_x = null_y = null_z = null_a = 0;

	if (0);
#define X(FUNC, NAME)\
	else if (!strcmp(argv[0], NAME))\
		kernel = FUNC(argc, argv + 1, &rows, &cols, &free_this);
	LIST_KERNELS
#undef X
	else
		eprintf("unrecognised kernel: %s\n", argv[0]);

	FPRINTF_HEAD(stdout, (size_t)1, cols, rows, "xyza");
	efflush(stdout, "<stdout>");

	buffer = emalloc2(cols, 4 * sizeof(double));
	n = cols * 4 * sizeof(double);

	kern = kernel;
	for (y = 0; y < rows; y++) {
		buf = buffer;
		for (x = 0; x < cols; x++) {
			buf[0] = null_x ? 0.0 : *kern;
			buf[1] = null_y ? 0.0 : *kern;
			buf[2] = null_z ? 0.0 : *kern;
			buf[3] = null_a ? 0.0 : *kern;
			buf += 4;
			kern++;
		}
		ewriteall(STDOUT_FILENO, buffer, n, "<stdout>");
	}

	free(buffer);
	free(free_this);
	return 0;
}
