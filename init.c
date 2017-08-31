/*
   Copyright 2017 James Hunt

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to
   deal in the Software without restriction, including without limitation the
   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
   sell copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software..

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.

   ---

   init - Run a command and restart it if it dies

   USAGE: init path/to/command args...

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PROGRAM "init"
#define EXIT_IMPROPER 1
#define EXIT_IN_CHILD 251

int main(int argc, char **argv)
{
	pid_t pid, kid;
	int rc;
	FILE *debug;

	if (argc < 2) {
		fprintf(stderr, "USAGE: init path/to/command args...\n");
		exit(EXIT_IMPROPER);
	}

	if (fcntl(3, F_GETFD) >= 0) {
		debug = fdopen(3, "w");
	} else {
		debug = fopen("/dev/null", "w");
	}

	freopen("/dev/null", "r", stdin);
rexec:
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, PROGRAM ": fork() failed: %s (error %d)\n", strerror(errno), errno);
		sleep(5);
		goto rexec;
	}
	if (pid == 0) {
		execvp(argv[1], &argv[1]);
		fprintf(stderr, PROGRAM ": failed to exec '%s': %s (error %d)\n", argv[1], strerror(errno), errno);
		exit(EXIT_IN_CHILD);
	}
	fprintf(debug, PROGRAM ": forked child process %d to run '%s'\n", pid, argv[1]);

	for (;;) {
		kid = waitpid(-1, &rc, 0);
		if (kid < 0) {
			fprintf(stderr, PROGRAM ": failed to waitpid(-1): %s (error %d)\n", strerror(errno), errno);
			break;
		}

		if (kid == pid) {
			if (WIFEXITED(rc) && WEXITSTATUS(rc) != EXIT_IN_CHILD) {
				fprintf(stderr, PROGRAM ": supervised child process %d exited with rc=%d\n", kid, WEXITSTATUS(rc));

			} else if (WIFSIGNALED(rc)) {
				fprintf(stderr, PROGRAM ": supervised child process %d killed with signal %d\n", kid, WTERMSIG(rc));

			} else {
				fprintf(stderr, PROGRAM ": supervised child process %d died with unrecognized status of %d (%08x)\n", kid, rc, rc);
			}

			sleep(5);
			goto rexec;
		}

		if (WIFEXITED(rc)) {
			fprintf(stderr, PROGRAM ": inherited child process %d exited with rc=%d\n", kid, WEXITSTATUS(rc));

		} else if (WIFSIGNALED(rc)) {
			fprintf(stderr, PROGRAM ": inherited child process %d killed with signal %d\n", kid, WTERMSIG(rc));

		} else {
			fprintf(stderr, PROGRAM ": inherited child process %d died with unrecognized status of %d (%08x)\n", kid, rc, rc);
		}
	}

	return 0;
}
