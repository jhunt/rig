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

   runas - Execute a program with a different real/effective UID/GID

   USAGE: runas user group path/to/command args...
          runas -v

 */

#define _BSD_SOURCE
#include "rig.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#define PROGRAM "runas"

static void
usage(int rc)
{
	fprintf(stderr, "USAGE: runas user group path/to/command args...\n");
	exit(rc);
}

static int
parseint(const char *s)
{
	int id;
	const char *p;

	id = 0;
	for (p = s; *p; p++) {
		if (*p < '0' || *p > '9') return -1;
		id = id * 10 + (*p - '0');
	}

	return id;
}

static uid_t
parseuid(const char *s)
{
	int id;
	struct passwd * pw;

	id = parseint(s);
	if (id >= 0) return id;

	pw = getpwnam(s);
	return pw ? pw->pw_uid : -1;
}

static gid_t
parsegid(const char *s)
{
	int id;
	struct group *gr;

	id = parseint(s);
	if (id >= 0) return id;

	gr = getgrnam(s);
	return gr ? gr->gr_gid : -1;
}

int main(int argc, char **argv)
{
	uid_t uid;
	gid_t gid;

	if (argc > 1 && argv[1][0] == '-') {
		if (eq(argv[1], "-v")) show_version(PROGRAM);
		if (eq(argv[1], "-h")) usage(EXIT_OK);
		usage(EXIT_IMPROPER);
	}
	if (argc < 4) usage(EXIT_IMPROPER);

	uid = parseuid(argv[1]);
	if (uid < 0) {
		fprintf(stderr, PROGRAM ": no such user '%s'\n", argv[1]);
		exit(EXIT_IMPROPER);
	}

	gid = parsegid(argv[2]);
	if (gid < 0) {
		fprintf(stderr, PROGRAM ": no such group '%s'\n", argv[2]);
		exit(EXIT_IMPROPER);
	}

	if (setregid(gid, gid) != 0) {
		fprintf(stderr, PROGRAM ": failed to set real/effective group id to %d (%s): %s (error %d)\n",
				gid, argv[2], strerror(errno), errno);
		exit(EXIT_RUNTIME);
	}

	if (setgroups(0, NULL) != 0) {
		fprintf(stderr, PROGRAM ": failed to clear auxiliary groups: %s (error %d)\n",
				strerror(errno), errno);
		exit(EXIT_RUNTIME);
	}

	if (setreuid(uid, uid) != 0) {
		fprintf(stderr, PROGRAM ": failed to set real/effective user id to %d (%s): %s (error %d)\n",
				uid, argv[1], strerror(errno), errno);
		exit(EXIT_RUNTIME);
	}

	execvp(argv[3], &argv[3]);
	fprintf(stderr, PROGRAM ": failed to exec '%s': %s (error %d)\n", argv[3], strerror(errno), errno);
	exit(EXIT_RUNTIME);
}
