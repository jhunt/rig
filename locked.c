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

   locked - Execute a program while holding a filesystem lock

   USAGE: locked lock-file path/to/command args...
          locked -v

 */

#include "rig.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>

#define PROGRAM "locked"

static void
usage(int rc)
{
	fprintf(stderr, "USAGE: locked lock-file path/to/command args...\n");
	exit(rc);
}

int main(int argc, char **argv)
{
	int fd;

	if (argc > 1 && argv[1][0] == '-') {
		if (eq(argv[1], "-v")) show_version(PROGRAM);
		if (eq(argv[1], "-h")) usage(EXIT_OK);
		usage(EXIT_IMPROPER);
	}
	if (argc < 3) usage(EXIT_IMPROPER);

	fd = open(argv[1], O_WRONLY | O_NDELAY | O_APPEND | O_CREAT, 0600);
	if (fd < 0) {
		fprintf(stderr, PROGRAM ": failed to lock '%s': %s (error %d)\n", argv[1], strerror(errno), errno);
		exit(EXIT_RUNTIME);
	}

	if (flock(fd, LOCK_EX) != 0) {
		fprintf(stderr, PROGRAM ": failed to lock '%s': %s (error %d)\n", argv[1], strerror(errno), errno);
		exit(EXIT_RUNTIME);
	}

	execvp(argv[2], &argv[2]);
	fprintf(stderr, PROGRAM ": failed to exec '%s': %s (error %d)\n", argv[2], strerror(errno), errno);
	exit(EXIT_RUNTIME);
}
