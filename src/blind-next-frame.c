/* See LICENSE file for copyright and license details. */
#include "stream.h"
#include "util.h"

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

USAGE("[-f frames] width height pixel-format ...")

int
main(int argc, char *argv[])
{
	struct stream stream;
	size_t n, w, h = 0;
	int i, anything = 0;
	char *p;

	stream.frames = 1;

	ARGBEGIN {
	case 'f':
		stream.frames = etozu_flag('f', UARGF(), 1, SIZE_MAX);
		break;
	default:
		usage();
	} ARGEND;

	if (argc < 3)
		usage();

	stream.fd = STDIN_FILENO;
	stream.file = "<stdin>";
	stream.pixfmt[0] = '\0';

	stream.width  = entozu_arg(2, "the width",  argv[0], 1, SIZE_MAX);
	stream.height = entozu_arg(2, "the height", argv[1], 1, SIZE_MAX);
	argv += 2, argc -= 2;

	n = (size_t)argc - 1;
	for (i = 0; i < argc; i++)
		n += strlen(argv[i]);
	if (n < sizeof(stream.pixfmt)) {
		p = stpcpy(stream.pixfmt, argv[0]);
		for (i = 1; i < argc; i++) {
			*p++ = ' ';
			p = stpcpy(p, argv[i]);
		}
	}

	enset_pixel_size(2, &stream);

	fprint_stream_head(stdout, &stream);
	enfflush(2, stdout, "<stdout>");

	w = stream.width * stream.pixel_size;
	for (; stream.frames; stream.frames--) {
		for (h = stream.height; h; h--) {
			for (n = w; n; n -= stream.ptr) {
				stream.ptr = 0;
				if (!enread_stream(2, &stream, n))
					goto done;
				anything = 1;
				enwriteall(2, STDOUT_FILENO, stream.buf, stream.ptr, "<stdout>");
			}
		}
	}
done:

	if (anything && stream.frames)
		enprintf(2, "%s: is shorted than expected\n", stream.file);

	return !anything;
}
