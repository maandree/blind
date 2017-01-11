/* See LICENSE file for copyright and license details. */
#include <fcntl.h>

#define eopen(...)  enopen(1, __VA_ARGS__)
#define enopen(...) xenopen(__VA_ARGS__, 0)

static inline int
xenopen(int status, const char *path, int flags, int mode, ...)
{
	int fd = open(path, flags, mode);
	if (fd < 0)
		enprintf(status, "open %s:", path);
	return fd;
}
