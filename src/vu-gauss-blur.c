/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

USAGE("[-j jobs] [-ac] sd-stream")

static int chroma = 0;
static int noalpha = 0;
static size_t jobs = 1;

static void
process_xyza(char *restrict output, char *restrict cbuf, char *restrict sbuf,
	     struct stream *colour, struct stream *sigma, size_t n, size_t unused)
{
	typedef double pixel_t[4];

	pixel_t *restrict clr = (pixel_t *)cbuf;
	pixel_t *restrict sig = (pixel_t *)sbuf;
	pixel_t *img = (pixel_t *)output;
	pixel_t c, k;
	double *p;
	size_t x1, y1, i1, x2, y2, i2;
	double dy, d, m, dblurred;
	int i, blurred, blur[3] = {0, 0, 0};
	size_t start = 0, end = colour->height;
	int is_master;

	/* premultiply alpha channel */
	if (!noalpha) {
		for (y1 = i1 = 0; y1 < colour->height; y1++) {
			for (x1 = 0; x1 < colour->width; x1++, i1++) {
				clr[i1][0] *= clr[i1][3];
				clr[i1][1] *= clr[i1][3];
				clr[i1][2] *= clr[i1][3];
			}
		}
	}

	is_master = efork_jobs(&start, &end, jobs);

	/* blur */
	i1 = start * colour->width;
	for (y1 = i1 = start; y1 < end; y1++) {
		for (x1 = 0; x1 < colour->width; x1++, i1++) {
			if (sig[i1][3] == 0)
				goto no_blur;
			if (!chroma) {
				for (i = 0; i < 3; i++) {
					k[i] = sig[i1][i] * sig[i1][3], c[i] = k[i] *= k[i] * 2, c[i] *= M_PI;
					k[i] = 1 / k[i], c[i] = -1 / c[i];
					blur[i] = !sig[i1][1];
				}
			} else {
				k[1] = sig[i1][1] * sig[i1][3], c[1] = k[1] *= k[1] * 2, c[1] *= M_PI;
				k[1] = 1 / -k[1], c[1] = 1 / c[1];
				blur[1] = !sig[i1][1];
			}
			if (blur[0] + blur[1] + blur[2] == 0)
				goto no_blur;

			p = img[i1];
			p[0] = p[1] = p[2] = 0;
			p[3] = noalpha;
			if (k[0] == k[1] && k[1] == k[2]) {
				for (y2 = i2 = 0; y2 < colour->height; y2++) {
					dy = (ssize_t)y1 - (ssize_t)y2;
					dy *= dy;
					for (x2 = 0; x2 < colour->width; x2++, i2++) {
						d = (ssize_t)x1 - (ssize_t)x2;
						d = d * d + dy;
						m = c[i1] * exp(d * k[i1]);
						for (i = noalpha ? 3 : 4; i--;)
							p[i] += clr[i2][i] * m;
					}
				}
			} else {
				blurred = 0;
				for (i = 0; i < n; i++) {
					if (!blur[i]) {
						p[i] = clr[i1][i];
						continue;
					}
					for (y2 = i2 = 0; y2 < colour->height; y2++) {
						dy = (ssize_t)y1 - (ssize_t)y2;
						dy *= dy;
						if (!noalpha) {
							for (x2 = 0; x2 < colour->width; x2++, i2++) {
								d = (ssize_t)x1 - (ssize_t)x2;
								d = d * d + dy;
								m = c[i1] * exp(d * k[i1]);
								p[i] += clr[i2][i] * m;
								p[3] += clr[i2][3] * m;
							}
						} else {
							for (x2 = 0; x2 < colour->width; x2++, i2++) {
								d = (ssize_t)x1 - (ssize_t)x2;
								d = d * d + dy;
								m = c[i1] * exp(d * k[i1]);
								p[i] += clr[i2][i] * m;
							}
						}
					}
					blurred += 1;
				}
				if (!noalpha) {
					dblurred = blurred;
					for (y2 = i2 = 0; y2 < colour->height; y2++)
						for (x2 = 0; x2 < colour->width; x2++, i2++)
							p[3] /= dblurred;
				}
			}

			continue;
		no_blur:
			img[i1][0] = clr[i1][0];
			img[i1][1] = clr[i1][1];
			img[i1][2] = clr[i1][2];
			img[i1][3] = clr[i1][3];
		}
	}

	ejoin_jobs(is_master);

	/* unpremultiply alpha channel */
	if (!noalpha) {
		for (y1 = i1 = 0; y1 < colour->height; y1++) {
			for (x1 = 0; x1 < colour->width; x1++, i1++) {
				if (!img[i1][3])
					continue;
				img[i1][0] /= img[i1][3];
				img[i1][1] /= img[i1][3];
				img[i1][2] /= img[i1][3];
			}
		}
	}

	(void) unused;
}

int
main(int argc, char *argv[])
{
	struct stream colour, sigma;
	void (*process)(char *restrict output, char *restrict cbuf, char *restrict sbuf,
			struct stream *colour, struct stream *sigma, size_t cn, size_t sn);

	ARGBEGIN {
	case 'a':
		noalpha = 1;
		break;
	case 'c':
		chroma = 1;
		break;
	case 'j':
		jobs = etozu_flag('j', EARG(), 1, SHRT_MAX);
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 1)
		usage();

	colour.file = "<stdin>";
	colour.fd = STDIN_FILENO;
	einit_stream(&colour);

	sigma.file = argv[0];
	sigma.fd = eopen(sigma.file, O_RDONLY);
	einit_stream(&sigma);

	if (!strcmp(colour.pixfmt, "xyza"))
		process = process_xyza;
	else
		eprintf("pixel format %s is not supported, try xyza\n", colour.pixfmt);

	echeck_compat(&colour, &sigma);

	process_each_frame_two_streams(&colour, &sigma, STDOUT_FILENO, "<stdout>", process);

	return 0;
}
