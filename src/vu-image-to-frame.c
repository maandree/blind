/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "util.h"

#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

static double
get_value(void *buffer)
{
	unsigned char *buf = buffer;
	unsigned long int value;
	double ret;
	value  = (unsigned long int)(buf[0]) << 12;
	value += (unsigned long int)(buf[1]) <<  8;
	value += (unsigned long int)(buf[2]) <<  4;
	value += (unsigned long int)(buf[3]);
	ret = value;
	value = 1UL << 15;
	value |= value - 1;
	ret /= value;
	return ret;
}

static void
usage(void)
{
	eprintf("usage: %s\n", argv0);
}

int
main(int argc, char *argv[])
{
	int pipe_rw[2];
	int i, old_fd;
	pid_t pid;
	int status;
	char buf[8096];
	size_t ptr, ptw, n;
	char *p;
	ssize_t r;
	double red, green, blue, pixel[4];
	char width[3 * sizeof(size_t) + 1] = {0};
	char height[3 * sizeof(size_t) + 1] = {0};

	ARGBEGIN {
	default:
		usage();
	} ARGEND;
	if (argc)
		usage();

	if (pipe(pipe_rw))
		eprintf("pipe:");

	if (pipe_rw[0] == STDIN_FILENO || pipe_rw[1] == STDIN_FILENO)
		eprintf("no stdin open\n");
	if (pipe_rw[0] == STDOUT_FILENO || pipe_rw[1] == STDOUT_FILENO)
		eprintf("no stdout open\n");
	for (i = 0; i < 2; i++) {
		if (pipe_rw[i] == STDERR_FILENO) {
			pipe_rw[i] = dup(old_fd = pipe_rw[i]);
			if (pipe_rw[i] < 0)
				eprintf("dup:");
			close(old_fd);
		}
	}

	pid = fork();
	if (pid < 0)
		eprintf("fork:");

	if (!pid) {
		close(pipe_rw[0]);
		if (dup2(pipe_rw[1], STDOUT_FILENO) < 0)
			eprintf("dup2:");
		close(pipe_rw[1]);
		/* XXX Is there a way to convert directly to raw XYZ? (Would avoid gamut truncation) */
		execlp("convert", "convert", "-", "-depth", "32", "-alpha", "activate", "pam:-", NULL);
		eprintf("exec convert:");
	}

	close(pipe_rw[1]);

	for (ptr = 0;;) {
		r = read(pipe_rw[0], buf + ptr, sizeof(buf) - ptr);
		if (r < 0)
			eprintf("read <subprocess>:");
		if (r == 0)
			eprintf("convertion failed\n");
		ptr += (size_t)r;

		for (;;) {
			p = memchr(buf, '\n', ptr);
			if (!p) {
				if (ptr == sizeof(buf))
					eprintf("convertion failed\n");
				break;
			}
			*p++ = '\0';
			if (strstr(buf, "WIDTH ") == buf) {
				if (*width || !buf[6] || strlen(buf + 6) >= sizeof(width))
					eprintf("convertion failed\n");
				strcpy(width, buf + 6);
			} else if (strstr(buf, "HEIGHT ") == buf) {
				if (*height || !buf[7] || strlen(buf + 7) >= sizeof(height))
					eprintf("convertion failed\n");
				strcpy(height, buf + 7);
			} else if (!strcmp(buf, "ENDHDR")) {
				memmove(buf, p, ptr -= (size_t)(p - buf));
				goto header_done;
			}
			memmove(buf, p, ptr -= (size_t)(p - buf));
		}
	}
header_done:

	if (!*width || !*height)
		eprintf("convertion failed\n");

	printf("%s %s xyza\n%cuivf", width, height, 0);
	fflush(stdout);
	if (ferror(stdout))
		eprintf("<stdout>:");

	for (;;) {
		for (ptr = 0; ptr + 15 < n; ptr += 16) {
			red      = get_value(buf + ptr +  0);
			green    = get_value(buf + ptr +  4);
			blue     = get_value(buf + ptr +  8);
			pixel[3] = get_value(buf + ptr + 12);

			srgb_to_ciexyz(red, green, blue, pixel + 0, pixel + 1, pixel + 2);

			for (ptw = 0; ptw < sizeof(pixel); ptw += (size_t)r) {
				r = write(STDOUT_FILENO, (char *)pixel + ptw, sizeof(pixel) - ptw);
				if (r < 0)
					eprintf("write <stdout>:");
			}
		}
		r = read(pipe_rw[0], buf, sizeof(buf));
		if (r < 0)
			eprintf("read <subprocess>:");
		if (r == 0)
			break;
		n = (size_t)r;
	}

	while (waitpid(pid, &status, 0) != pid);
	return !!status;
}
