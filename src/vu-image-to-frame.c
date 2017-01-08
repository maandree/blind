/* See LICENSE file for copyright and license details. */
#include "arg.h"
#include "util.h"

#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

static char *
xmemmem(char *h, const char *n, size_t hn, size_t nn)
{
	char *p, *end;
	if (nn > hn)
		return NULL;
	end = h + (hn - nn + 1);
	for (p = h; p != end; p++) {
		p = memchr(p, *n, (size_t)(end - p));
		if (!p)
			return NULL;
		if (!memcmp(p, n, nn))
			return p;
	}
	return NULL;
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
	size_t ptr, n;
	char *p;
	ssize_t r;

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
		execlp("convert", "convert", "-", "-depth", "8", "-alpha", "activate", "pam:-", NULL);
		eprintf("exec convert:");
	}
	
	close(pipe_rw[1]);

	for (ptr = 0;;) {
		r = read(pipe_rw[0], buf + ptr, sizeof(buf) - ptr);
		if (r < 0)
			eprintf("read <subprocess>:");
		if (r == 0)
			break;
		ptr += (size_t)r;
		p = xmemmem(buf, "\nENDHDR\n", ptr, sizeof("\nENDHDR\n") - 1);
		if (!p)
			continue;
		p += sizeof("\nENDHDR\n") - 1;
		n = (size_t)(p - buf);
		memmove(buf, buf + n, ptr - n);
		n = ptr - n;
		break;
	}

	for (;;) {
		for (ptr = 0; ptr < n;) {
			r = write(STDOUT_FILENO, buf + ptr, n - ptr);
			if (r < 0)
				eprintf("write <stdout>:");
			ptr += (size_t)r;
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
