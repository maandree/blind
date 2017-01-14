/* See LICENSE file for copyright and license details. */
#include "util.h"

#if defined(HAVE_PRCTL)
# include <sys/prctl.h>
#endif
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

char *argv0;

void
weprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	if (argv0 && strncmp(fmt, "usage", strlen("usage")))
		fprintf(stderr, "%s: ", argv0);

	vfprintf(stderr, fmt, ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	}

	va_end(ap);
}


int
tollu(const char *s, unsigned long long int min, unsigned long long int max, unsigned long long int *out)
{
	char *end;
	errno = 0;
	if (*s == '-') {
		errno = ERANGE;
		return -1;
	}
	if (tolower(*s) == 'x' || *s == '#')
		*out = strtoull(s + 1, &end, 16);
	else if (*s == '0' && tolower(s[1]) == 'x')
		*out = strtoull(s + 2, &end, 16);
	else if (*s == '0' && tolower(s[1]) == 'o')
		*out = strtoull(s + 2, &end, 8);
	else if (*s == '0' && tolower(s[1]) == 'b')
		*out = strtoull(s + 2, &end, 2);
	else
		*out = strtoull(s, &end, 10);
	if (errno)
		return -1;
	if (*end) {
		errno = EINVAL;
		return -1;
	}
	if (*out < min || *out > max) {
		errno = ERANGE;
		return -1;
	}
	return 0;
}

int
tolli(const char *s, long long int min, long long int max, long long int *out)
{
	int sign = 1;
	unsigned long long int inter;
	errno = 0;
	if (*s == '-') {
		s++;
		sign = -1;
	}
	if (tollu(s, 0, ULLONG_MAX, &inter))
		return -1;
	if (sign > 0) {
		if (max < 0 || inter > (unsigned long long int)max)
			goto erange;
		*out = (long long int)inter;
		if (*out < min)
			goto erange;
	} else {
#if LLONG_MIN == -LLONG_MAX
		if (inter > -LLONG_MIN)
			goto erange;
#else
		if (inter > (unsigned long long int)LLONG_MAX + 1ULL)
			goto erange;
#endif
		*out = -(long long int)inter;
		if (*out < min || *out > max)
			goto erange;
	}
	return 0;

erange:
	errno = ERANGE;
	return -1;
}


int
writeall(int fd, void *buf, size_t n)
{
	char *buffer = buf;
	ssize_t r;
	while (n) {
		r = write(fd, buffer, n);
		if (r < 0)
			return -1;
		buffer += (size_t)r;
		n -= (size_t)r;
	}
	return 0;
}

ssize_t
readall(int fd, void *buf, size_t n)
{
	char *buffer = buf;
	size_t ptr = 0;
	ssize_t r;
	for (;;) {
		r = read(fd, buffer + ptr, n - ptr);
		if (r < 0)
			return -1;
		if (r == 0)
			break;
		r += (size_t)r;
	}
	return r;
}

int
pwriteall(int fd, void *buf, size_t n, size_t ptr)
{
	char *buffer = buf;
	ssize_t r;
	while (n) {
		r = pwrite(fd, buffer, n, ptr);
		if (r < 0)
			return -1;
		buffer += (size_t)r;
		n -= (size_t)r;
		ptr += (size_t)r;
	}
	return 0;
}


static inline pid_t
enfork(int status)
{
	pid_t pid = fork();
	if (pid == -1)
		enprintf(status, "fork:");
	return pid;
}


int
enfork_jobs(int status, size_t *start, size_t *end, size_t jobs)
{
	size_t j, s = *start, n = *end - *start;
	if (jobs < 2)
		return 1;
	*end = n / jobs + s;
	for (j = 1; j < jobs; j++) {
		if (!enfork(status)) {
#if defined(HAVE_PRCTL) && defined(PR_SET_PDEATHSIG)
			prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
			*start = n * (j + 0) / jobs + s;
			*end   = n * (j + 1) / jobs + s;
			return 0;
		}
	}
	return 1;
}

void
enjoin_jobs(int status, int is_master)
{
	int stat;
	if (!is_master)
		exit(0);
	while (wait(&stat) != -1)
		if (!stat)
			exit(status);
	if (errno != ECHILD)
		enprintf(status, "wait:");
}
