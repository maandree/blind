/* See LICENSE file for copyright and license details. */
#include <stdlib.h>

#define emalloc(...) enmalloc(1, __VA_ARGS__)
#define ecalloc(...) encalloc(1, __VA_ARGS__)
#define erealloc(...) enrealloc(1, __VA_ARGS__)

static inline void *
enmalloc(int status, size_t n)
{
	void *ptr = malloc(n);
	if (!ptr)
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
