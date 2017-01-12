/* See LICENSE file for copyright and license details. */

#define ewriteall(...) enwriteall(1, __VA_ARGS__)
#define ereadall(...)  enreadall(1, __VA_ARGS__)

int writeall(int fd, void *buf, size_t n);

static inline void
enwriteall(int status, int fd, void *buf, size_t n, const char *fname)
{
	if (writeall(fd, buf, n))
		enprintf(status, "write %s:", fname);
}

ssize_t readall(int fd, void *buf, size_t n);

static inline size_t
enreadall(int status, int fd, void *buf, size_t n, const char *fname)
{
	ssize_t r = readall(fd, buf, n);
	if (r < 0)
		enprintf(status, "read %s:", fname);
	return (size_t)r;
}
