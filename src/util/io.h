/* See LICENSE file for copyright and license details. */

#define ewriteall(...) enwriteall(1, __VA_ARGS__)

int writeall(int fd, void *buf, size_t n);

static inline void
enwriteall(int status, int fd, void *buf, size_t n, const char *fname)
{
	if (writeall(fd, buf, n))
		enprintf(status, "write %s:", fname);
}
