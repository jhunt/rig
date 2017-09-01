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

   supervise - Supervise services in a directory, with resurrection, log file
               management and more.

   USAGE: supervise /path/to/services [/another/path ...]
          supervise -v

 */

#include "rig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>

#define PROGRAM "supervise"

#define MAX_SERVICES 5000
#define MAX_FILENAME 8192
#define MIN_FILENAME 3

struct {
	dev_t dev;
	ino_t ino;
	pid_t pid;
} services[MAX_SERVICES];
int nservices = 0;
char path[MAX_FILENAME];

static int
find(struct stat *st)
{
	int i;

	for (i = 0; i < nservices; i++)
		if (services[i].ino == st->st_ino && services[i].dev == st->st_dev)
			return i;

	if (nservices == MAX_SERVICES) return -1;
	i = nservices++;

	services[i].dev = st->st_dev;
	services[i].ino = st->st_ino;
	services[i].pid = 0;
	return i;
}

static void
runone(const char *bin)
{
	int rc, i;
	struct stat st;

	/* ignore NULL argument (unlikely and probably a bug)
	   and all hidden files / directories (likely). */
	if (!bin || bin[0] == '.') return;

	/* we should be chdir()'d into the container directory
	   which simplifies the path shenanigans we have to endure */
	rc = snprintf(path, MAX_FILENAME, "./%s", bin);
	if (rc <= MIN_FILENAME) {
		fprintf(stderr, PROGRAM ": snprintf() failed\n");
		exit(EXIT_RUNTIME);
	}

	/* check to see if the file exists */
	rc = stat(path, &st);
	if (rc < 0) {
		fprintf(stderr, PROGRAM ": failed to stat %s: %s (error %d); skipping it.\n", bin, strerror(errno), errno);
		return;
	}

	/* skip irregular files outright */
	if (!S_ISREG(st.st_mode)) return;

	/* skip non-executable regular files */
	rc = access(path, X_OK);
	if (rc != 0 && errno == EACCES) return;

	i = find(&st);
	if (i < 0) {
		fprintf(stderr, PROGRAM ": could not start %s: too many services.\n", bin);
		return;
	}

	/* start services[i] if it is not already running */
	if (services[i].pid == 0) {
		pid_t pid;
		char *argv[3];

		pid = fork();
		if (pid < 0) {
			fprintf(stderr, PROGRAM ": unable to fork(): %s (error %d)\n", strerror(errno), errno);
			return;
		}
		if (pid == 0) {
			argv[0] = "always";
			argv[1] = path;
			argv[2] = NULL;
			execvp(argv[0], argv);
			exit(EXIT_IN_CHILD);
		}
		services[i].pid = pid;
	}
}

static void
runall(void)
{
	DIR *d;
	struct dirent *e;

	d = opendir(".");
	if (!d) {
		*path = '\0';
		getcwd(path, MAX_FILENAME);
		fprintf(stderr, PROGRAM ": failed to read %s: %s (error %d)\n", path, strerror(errno), errno);
		exit(EXIT_RUNTIME);
	}

	while ((e = readdir(d)) != NULL)
		runone(e->d_name);

	closedir(d);
}

static void
reapall(void)
{
	int i, status;
	pid_t pid;

	for (;;) {
		pid = waitpid(-1, &status, WNOHANG);
		if (pid == 0) break;
		if (pid < 0) {
			if (errno == ECHILD) break;
			fprintf(stderr, PROGRAM ": waitpid() failed: %s (error %d)\n", strerror(errno), errno);
			break;
		}

		for (i = 0; i < nservices; i++)
			if (services[i].pid == pid)
				services[i].pid = 0;
	}
}

static void
usage(int rc)
{
	fprintf(stderr, "USAGE: " PROGRAM " /path/to/services\n");
	exit(EXIT_IMPROPER);
}

int main(int argc, char **argv)
{
	int rc;

	if (argc != 2) usage(EXIT_IMPROPER);
	if (argv[1][0] == '-') {
		if (eq(argv[1], "-v")) show_version(PROGRAM);
		if (eq(argv[1], "-h")) usage(EXIT_OK);
		usage(EXIT_IMPROPER);
	}

	/* check for absolute paths */
	if (argv[1][0] != '/') {
		fprintf(stderr, PROGRAM ": %s is not an absolute path\n", argv[1]);
		exit(EXIT_IMPROPER);
	}

	rc = chdir(argv[1]);
	if (rc != 0) {
		fprintf(stderr, PROGRAM ": failed to chdir to %s: %s (error %d)\n", argv[1], strerror(errno), errno);
		exit(EXIT_RUNTIME);
	}

	for (;;) {
		reapall();
		runall();
		sleep(2);
	}

	return 0;
}
