/* See LICENSE file for copyright and license details. */
#include "util.h"

#include <arpa/inet.h>
#include <sys/wait.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

USAGE("[-h] [-f | -p]")

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

int
main(int argc, char *argv[])
{
	int pipe_rw[2];
	int i, old_fd;
	pid_t pid = 0;
	int status;
	char buf[8096];
	size_t ptr, n;
	char *p;
	ssize_t r;
	double red, green, blue, pixel[4];
	char width[3 * sizeof(size_t) + 1] = {0};
	char height[3 * sizeof(size_t) + 1] = {0};
	int headless = 0, farbfeld = 0, pam = 0;
	const char *file = "<subprocess>";
	const char *conv_fail_msg = "convertion failed, if converting a farbfeld file, try -f";

	ARGBEGIN {
	case 'f':
		farbfeld = 1;
		break;
	case 'h':
		headless = 1;
		break;
	case 'p':
		pam = 1;
		break;
	default:
		usage();
	} ARGEND;

	if (argc || (farbfeld && pam))
		usage();

	if (farbfeld || pam) {
		if (farbfeld)
			conv_fail_msg = "not a valid farbfeld file, try without -f";
		else
			conv_fail_msg = "not a valid 16-bit RGBA portable arbitrary map file, try without -p";
		file = "<stdin>";
		pipe_rw[0] = STDIN_FILENO;
		goto after_fork;
	}

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
after_fork:

	if (farbfeld) {
		for (ptr = 0; ptr < 16; ptr++) {
			r = read(pipe_rw[0], buf + ptr, sizeof(buf) - ptr);
			if (r < 0)
				eprintf("read %s:", file);
			if (r == 0)
				eprintf("%s\n", conv_fail_msg);
			ptr += (size_t)r;
		}
		if (memcmp(buf, "farbfeld", 8))
			eprintf("%s\n", conv_fail_msg);
		sprintf(width,  "%"PRIu32, ntohl(*(uint32_t *)(buf +  8)));
		sprintf(height, "%"PRIu32, ntohl(*(uint32_t *)(buf + 12)));
		ptr = 0;
		goto header_done;
	}

	for (ptr = 0;;) {
		r = read(pipe_rw[0], buf + ptr, sizeof(buf) - ptr);
		if (r < 0)
			eprintf("read %s:", file);
		if (r == 0)
			eprintf("%s\n", conv_fail_msg);
		ptr += (size_t)r;

		for (;;) {
			p = memchr(buf, '\n', ptr);
			if (!p) {
				if (ptr == sizeof(buf))
					eprintf("%s\n", conv_fail_msg);
				break;
			}
			*p++ = '\0';
			if (strstr(buf, "WIDTH ") == buf) {
				if (*width || !buf[6] || strlen(buf + 6) >= sizeof(width))
					eprintf("%s\n", conv_fail_msg);
				strcpy(width, buf + 6);
			} else if (strstr(buf, "HEIGHT ") == buf) {
				if (*height || !buf[7] || strlen(buf + 7) >= sizeof(height))
					eprintf("%s\n", conv_fail_msg);
				strcpy(height, buf + 7);
			} else if (!strcmp(buf, "ENDHDR")) {
				memmove(buf, p, ptr -= (size_t)(p - buf));
				goto header_done;
			}
			memmove(buf, p, ptr -= (size_t)(p - buf));
		}
	}
header_done:
	n = ptr;

	if (!*width || !*height)
		eprintf("%s\n", conv_fail_msg);

	if (!headless) {
		printf("1 %s %s xyza\n%cuivf", width, height, 0);
		efflush(stdout, "<stdout>");
	}

	for (;;) {
		for (ptr = 0; ptr + 15 < n; ptr += 16) {
			red      = srgb_decode(get_value(buf + ptr + 0));
			green    = srgb_decode(get_value(buf + ptr + 4));
			blue     = srgb_decode(get_value(buf + ptr + 8));
			pixel[3] = get_value(buf + ptr + 12);

			srgb_to_ciexyz(red, green, blue, pixel + 0, pixel + 1, pixel + 2);
			ewriteall(STDOUT_FILENO, pixel, sizeof(pixel), "<stdout>");
		}
		r = read(pipe_rw[0], buf, sizeof(buf));
		if (r < 0)
			eprintf("read %s:", file);
		if (r == 0)
			break;
		n = (size_t)r;
	}

	if (farbfeld || pam)
		return 0;
	close(pipe_rw[0]);
	while (waitpid(pid, &status, 0) != pid);
	return !!status;
}
