/* See LICENSE file for copyright and license details. */
#include "util.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

char *argv0;

static void
xvprintf(const char *fmt, va_list ap)
{
	if (argv0 && strncmp(fmt, "usage", strlen("usage")))
		fprintf(stderr, "%s: ", argv0);

	vfprintf(stderr, fmt, ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	}
}

void
eprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	xvprintf(fmt, ap);
	va_end(ap);

	exit(1);
}

void
enprintf(int status, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	xvprintf(fmt, ap);
	va_end(ap);

	exit(status);
}

void
weprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	xvprintf(fmt, ap);
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
	char *end;
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
fshut(FILE *fp, const char *fname)
{
 	int ret = 0;

        /* fflush() is undefined for input streams by ISO C,
         * but not POSIX 2008 if you ignore ISO C overrides.
         * Leave it unchecked and rely on the following
         * functions to detect errors.
         */
        fflush(fp);

        if (ferror(fp) && !ret) {
        	weprintf("ferror %s:", fname);
                ret = 1;
        }

        if (fclose(fp) && !ret) {
                weprintf("fclose %s:", fname);
		ret = 1;
	}

        return ret;
}

void
enfshut(int status, FILE *fp, const char *fname)
{
 	if (fshut(fp, fname))
                exit(status);
}

void
efshut(FILE *fp, const char *fname)
{
        enfshut(1, fp, fname);
}
