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

   logto - Timestamp lines read from standard input, and write them to disk

   USAGE: ./some/program | logto /the/log/file
          logto -v

 */

#include "rig.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#define PROGRAM "logto"

#define MAX_LINE 8192

static void
writeall(int fd, const void *buf, size_t count)
{
	ssize_t n;
	size_t off;

	off = 0;
again:
	n = write(fd, (const char *)buf + off, count - off);
	if (n <= 0) return;
	off += n;
	goto again;
}

int main(int argc, char **argv)
{
	int rc, mid;
	char *a, *b, buf[MAX_LINE], ts[16];
	struct timeval t;
	ssize_t nread;

	if (argc != 2) {
		fprintf(stderr, "USAGE: log /path/to/log/file\n");
		exit(EXIT_IMPROPER);
	}

	if (argv[1][0] == '-') {
		if (eq(argv[1], "-v")) show_version(PROGRAM);
		fprintf(stderr, "USAGE: log /path/to/log/file\n");
		exit(EXIT_IMPROPER);
	}

	if (!freopen(argv[1], "a", stdout)) {
		fprintf(stderr, "%s: %s (error %d)\n", argv[1], strerror(errno), errno);
		exit(EXIT_RUNTIME);
	}

	memset(ts, ' ', 16);
	mid = 0;
	for (;;) {
		nread = read(0, buf, MAX_LINE - 1);
		if (nread == 0) break;
		if (nread < 0) {
			fprintf(stderr, "failed to read from stdin: %s (error %d)\n", strerror(errno), errno);
			exit(EXIT_RUNTIME);
		}
		buf[nread] = '\0';

		rc = gettimeofday(&t, NULL);
		if (rc < 0) {
			fprintf(stderr, "failed to get current time: %s (error %d)\n", strerror(errno), errno);
			exit(EXIT_RUNTIME);
		}
		ts[9] = '0' + t.tv_sec % 10; t.tv_sec /= 10;
		ts[8] = '0' + t.tv_sec % 10; t.tv_sec /= 10;
		ts[7] = '0' + t.tv_sec % 10; t.tv_sec /= 10;
		ts[6] = '0' + t.tv_sec % 10; t.tv_sec /= 10;
		ts[5] = '0' + t.tv_sec % 10; t.tv_sec /= 10;
		ts[4] = '0' + t.tv_sec % 10; t.tv_sec /= 10;
		ts[3] = '0' + t.tv_sec % 10; t.tv_sec /= 10;
		ts[2] = '0' + t.tv_sec % 10; t.tv_sec /= 10;
		ts[1] = '0' + t.tv_sec % 10; t.tv_sec /= 10;
		ts[0] = '0' + t.tv_sec % 10;

		ts[10] = '.';

		t.tv_usec /= 1000;
		ts[14] = '0' + t.tv_usec % 10; t.tv_usec /= 10;
		ts[13] = '0' + t.tv_usec % 10; t.tv_usec /= 10;
		ts[12] = '0' + t.tv_usec % 10; t.tv_usec /= 10;
		ts[11] = '0' + t.tv_usec % 10;

		for (a = &buf[0]; *a; ) {
			b = strchr(a, '\n');
			if (!b) {
				mid = 1;
				break;
			}

			if (!mid) writeall(1, ts, 16);
			writeall(1, a, b - a + 1);
			a = b + 1; mid = 0;
		}
	}

	return 0;
}
