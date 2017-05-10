/* See LICENSE file for copyright and license details. */
#include <stdlib.h>
#include <stdint.h>

#define emalloc(...)   enmalloc(1, __VA_ARGS__)
#define emalloc2(...)  enmalloc2(1, __VA_ARGS__)
#define ecalloc(...)   encalloc(1, __VA_ARGS__)
#define erealloc(...)  enrealloc(1, __VA_ARGS__)
#define erealloc2(...) enrealloc2(1, __VA_ARGS__)

#define malloc2(n, m)     malloc(n * m);
#define realloc3(p, n, m) realloc(p, n * m);

static inline void *
enmalloc(int status, size_t n)
{
	void *ptr = malloc(n);
	if (!ptr)
		enprintf(status, "malloc: out of memory\n");
	return ptr;
}

static inline void *
enmalloc2(int status, size_t n, size_t m)
{
	void *ptr;
	if (n > SIZE_MAX / m || !(ptr = malloc(n * m)))
		enprintf(status, "malloc: out of memory\n");
	return ptr;
}

static inline void *
encalloc(int status, size_t n, size_t m)
{
	void *ptr = calloc(n, m);
	if (!ptr)
		enprintf(status, "calloc: out of memory\n");
	return ptr;
}

static inline void *
enrealloc(int status, void *ptr, size_t n)
{
	ptr = realloc(ptr, n);
	if (!ptr)
		enprintf(status, "realloc: out of memory\n");
	return ptr;
}

static inline void *
enrealloc2(int status, void *ptr, size_t n, size_t m)
{
	if (n > SIZE_MAX / m || !(ptr = realloc(ptr, n * m)))
		enprintf(status, "realloc: out of memory\n");
	return ptr;
}
