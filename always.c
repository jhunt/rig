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

   always - Run a command and restart it if it dies

   USAGE: always path/to/command args...

 */

#include "rig.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PROGRAM "always"

#define TOOFAST 2
#define RESPAWN 5

static void
usage(int rc)
{
	fprintf(stderr, "USAGE: " PROGRAM " path/to/command args...\n");
	exit(rc);
}

int main(int argc, char **argv)
{
	pid_t pid, kid;
	int rc;
	FILE *debug;
	struct timespec now, last;

	if (argc < 2) usage(EXIT_IMPROPER);
	if (argv[1][0] == '-') {
		if (eq(argv[1], "-v")) show_version(PROGRAM);
		if (eq(argv[1], "-h")) usage(EXIT_OK);
		usage(EXIT_IMPROPER);
	}

	if (fcntl(3, F_GETFD) >= 0) {
		debug = fdopen(3, "w");
	} else {
		debug = fopen("/dev/null", "w");
	}

	freopen("/dev/null", "r", stdin);
	memset(&last, 0, sizeof(last));

reexec:
	clock_gettime(CLOCK_MONOTONIC, &last);

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, PROGRAM ": fork() failed: %s (error %d)\n", strerror(errno), errno);
		sleep(5);
		goto reexec;
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
				fprintf(stderr, PROGRAM ": process %d exited with rc=%d\n", kid, WEXITSTATUS(rc));

			} else if (WIFSIGNALED(rc)) {
				fprintf(stderr, PROGRAM ": process %d killed with signal %d\n", kid, WTERMSIG(rc));

			} else {
				fprintf(stderr, PROGRAM ": process %d died with unrecognized status of %d (%08x)\n", kid, rc, rc);
			}

			rc = clock_gettime(CLOCK_MONOTONIC, &now);
			if (rc < 0) {
				fprintf(stderr, PROGRAM ": failed to get current time: %s (error %d)\n", strerror(errno), errno);
				fprintf(stderr, PROGRAM ": waiting %d seconds to respawn...\n", RESPAWN);
				sleep(RESPAWN);
				goto reexec;
			}

			if (now.tv_sec < last.tv_sec + TOOFAST) {
				fprintf(stderr, PROGRAM ": process dying too quickly; waiting %d seconds to respawn...\n", RESPAWN);
				sleep(RESPAWN);
			}
			goto reexec;
		}
	}

	return 0;
}
