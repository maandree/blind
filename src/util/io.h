/* See LICENSE file for copyright and license details. */
#include <fcntl.h>

#if defined(POSIX_FADV_SEQUENTIAL)
# define fadvise_sequential(...)  posix_fadvise(__VA_ARGS__, POSIX_FADV_SEQUENTIAL)
#else
# define fadvise_sequential(...)
#endif

#if defined(POSIX_FADV_RANDOM)
# define fadvise_random(...)  posix_fadvise(__VA_ARGS__, POSIX_FADV_RANDOM)
#else
# define fadvise_random(...)
#endif

#define ewriteall(...)     enwriteall(1, __VA_ARGS__)
#define ereadall(...)      enreadall(1, __VA_ARGS__)
#define epwriteall(...)    enpwriteall(1, __VA_ARGS__)
#define ewritezeroes(...)  enwritezeroes(1, __VA_ARGS__)

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

int pwriteall(int fd, void *buf, size_t n, size_t ptr);

static inline void
enpwriteall(int status, int fd, void *buf, size_t n, size_t ptr, const char *fname)
{
	if (pwriteall(fd, buf, n, ptr))
		enprintf(status, "pwrite %s:", fname);
}

int writezeroes(int fd, void *buf, size_t bufsize, size_t n);

static inline void
enwritezeroes(int status, int fd, void *buf, size_t bufsize, size_t n, const char *fname)
{
	if (writezeroes(fd, buf, bufsize, n))
		enprintf(status, "write %s:", fname);
}
