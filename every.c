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

   every - Run a command on a periodic schedule

   USAGE: every N path/to/command args...
          every -v

   N must be in seconds, without a unit.  Only values between 1 and 86400
   (1 day) are allowed.  Values outside of that range will cause `every'
   to exit non-zero.

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

#define PROGRAM "every"

static void
oops_improper(const char *v)
{
	if (v) fprintf(stderr, PROGRAM ": invalid value for N '%s' (must be between 1-86400, inclusive)\n", v);
	else   fprintf(stderr, PROGRAM ": invalid value for N (must be between 1-86400, inclusive)\n");
	exit(EXIT_IMPROPER);
}

static void
oops_runtime(const char *s)
{
	if (s) fprintf(stderr, PROGRAM ": %s: %s (error %d)\n", s, strerror(errno), errno);
	exit(EXIT_RUNTIME);
}

int main(int argc, char **argv)
{
	pid_t pid;
	int n, rc, use_clock;
	char *p;
	struct timespec start, end, nap;
	FILE *debug;

	if (argc > 1 && argv[1][0] == '-') {
		if (eq(argv[1], "-v")) show_version(PROGRAM);
		fprintf(stderr, "USAGE: every N path/to/command args...\n");
		exit(EXIT_IMPROPER);
	}
	if (argc < 3) {
		fprintf(stderr, "USAGE: every N path/to/command args...\n"
		                "\n"
		                "N must be in seconds, without a unit.\n"
		                "Valid values are between 1 and 86400, inclusive.\n");
		exit(EXIT_IMPROPER);
	}

	if (fcntl(3, F_GETFD) >= 0) {
		debug = fdopen(3, "w");
	} else {
		debug = fopen("/dev/null", "w");
	}

	n = 0;
	p = argv[1];
	if (*p == '+') p++;
	for (; *p; p++) {
		if (*p >= '0' && *p <= '9') {
			n = n * 10 + (*p - '0');
			if (n > 86400) oops_improper(argv[1]);

		} else {
			oops_improper(argv[1]);
		}
	}

	if (!freopen("/dev/null", "r", stdin)) {
		fprintf(stderr, PROGRAM ": failed to redirect /dev/null into stdin\n");
		fclose(stdin);
	}
	for (;;) {
		use_clock = 1;
		rc = clock_gettime(CLOCK_MONOTONIC, &end);
		if (rc == 0) {
			end.tv_sec += n;
		} else {
			fprintf(stderr, PROGRAM ": failed to get the current time: %s (error %d)\n", strerror(errno), errno);
			use_clock = 0;
		}

		pid = fork();
		if (pid < 0) oops_runtime("fork() failed");
		if (pid == 0) {
			execvp(argv[2], &argv[2]);
			fprintf(stderr, PROGRAM ": failed to exec '%s': %s (error %d)\n", argv[2], strerror(errno), errno);
			exit(EXIT_IN_CHILD);
		}

		if (waitpid(pid, &rc, 0) < 0) oops_runtime("waitpid() failed");
		if (rc != 0) {
			if (WIFEXITED(rc) && WEXITSTATUS(rc) != EXIT_IN_CHILD) {
				fprintf(stderr, PROGRAM ": command '%s' exited with rc=%d\n", argv[2], WEXITSTATUS(rc));

			} else if (WIFSIGNALED(rc)) {
				fprintf(stderr, PROGRAM ": command '%s' killed with signal %d\n", argv[2], WTERMSIG(rc));

			} else {
				fprintf(stderr, PROGRAM ": command '%s' died with unrecognized status of %d (%08x)\n", argv[2], rc, rc);
			}
		}

		if (use_clock) {
			rc = clock_gettime(CLOCK_MONOTONIC, &start);
			if (rc < 0) {
				fprintf(stderr, PROGRAM ": failed to get the current time: %s (error %d)\n", strerror(errno), errno);
				sleep(n);
				continue;
			}
			if (end.tv_nsec < start.tv_nsec) {
				end.tv_sec--;
				end.tv_nsec += 1000000000;
			}
			nap.tv_sec  = end.tv_sec  - start.tv_sec;
			nap.tv_nsec = end.tv_nsec - start.tv_nsec;
naptime:
			fprintf(debug, PROGRAM ": sleeping for %8.3lfs\n", nap.tv_sec + (nap.tv_nsec / 1000000000.0));
			rc = nanosleep(&nap, &end);
			if (rc < 0) {
				if (errno == EINTR) {
					fprintf(debug, PROGRAM ": interrupted by signal; resuming nap...\n");
					memcpy(&nap, &end, sizeof(nap));
					goto naptime;
				}
				fprintf(stderr, PROGRAM ": failed to sleep: %s (error %d)\n", strerror(errno), errno);
			}

		} else {
			sleep(n);
		}
	}

	return 0;
}
