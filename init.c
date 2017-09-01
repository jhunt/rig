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

   init - An initial process for bootstrapping a running system

   USAGE: init [/etc/inittab]
          init -v

 */

#include "rig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PROGRAM "init"

/*
   `struct child` contains all of the details for each of the
   child processes that we are supervising.  Partially, this
   configuration comes from the inittab, but it also stores the
   running state of the child process, in the `pid` member.

   This is inherently list-like, since we probably need to
   supervisor more than one child process (otherwise, we wouldn't
   actually _need_ init in the first place, now would we?)
 */
struct child {
	struct child *next; /* the rest of the linked list */
	                    /* (NULL at end-of-list)       */

	char *command;      /* command script to run       */

	pid_t pid;          /* PID of the running process. */
	                    /* set to 0 for "not running"  */
};

/*
   Given the path to an inittab, parse the file and return a
   heap-allocated `child` structure that contains all of the
   pertinent details from the inittab.

   Handles comments, blank lines, etc.

   Any errors will cause the parsing to terminate, and a NULL
   pointer will be returned.  (i.e. NULL = bad config)
 */
struct child* configure(const char *path)
{
	FILE *f;
	char buf[8192], *p, *q;
	unsigned long line;
	struct child *chain, *next;

	f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "%s: %s\n", path, strerror(errno));
		return NULL;
	}

	line = 0;
	chain = next = NULL;
	while (fgets(buf, 8192, f) != NULL) {
		line++;
		p = strrchr(buf, '\n');
		if (!p && buf[8190]) {
			fprintf(stderr, "%s:%lu: line is too long!\n", path, line);
			return NULL;
		}
		if (p)
			*p = '\0';

		p = buf;
		while (*p && isspace(*p))
			p++;
		if (!*p || *p == '#')
			continue;

		if (*p != '/') {
			fprintf(stderr, "%s:%lu: command '%s' must be absolutely qualified\n",
			                path, line, p);
			return NULL;
		}

		for (q = p; *q && !isspace(*q) && isprint(*q); q++)
			;
		if (*q) {
			int pad = (int)(strlen(path) + (line / 10 + 1) + (q - p));
			fprintf(stderr, "%s:%lu: command '%s' looks suspicious\n"
			                "            %*s^~~ problem starts here...\n",
			                path, line, p,
			                pad, " ");
			return NULL;
		}

		if (next == NULL) {
			chain = next = calloc(1, sizeof(struct child));
		} else {
			struct child *tmp = next;
			next = calloc(1, sizeof(struct child));
			tmp->next = next;
		}
		next->command = strdup(p);
	}

	next = chain;
	while (next) {
		fprintf(stderr, "- [%s]\n", next->command);
		next = next->next;
	}

	fclose(f);
	if (!chain) {
		fprintf(stderr, "%s: no commands defined.\nWhat shall I supervise?\n", path);
		return NULL;
	}

	return chain;
}

void spin(struct child *config)
{
	config->pid = fork();

	if (config->pid < 0) {
		fprintf(stderr, "fork failed: %s\n", strerror(errno));
		return;
	}

	if (config->pid == 0) {
		/* in child process; set up for an exec! */
		char *argv[2] = { NULL, NULL };
		char *envp[1] = { NULL };
		argv[0] = strrchr(config->command, '/') + 1;

		if (!freopen("/dev/null", "r", stdin)) {
			fprintf(stderr, PROGRAM ": failed to redirect /dev/null into stdin\n");
			fclose(stdin);
		}
		if (!freopen("/dev/null", "w", stdout)) {
			fprintf(stderr, PROGRAM ": failed to redirect stdout to /dev/null\n");
			fclose(stdout);
		}
		if (!freopen("/dev/null", "w", stderr)) {
			fprintf(stderr, PROGRAM ": failed to redirect stderr to /dev/null\n");
			fclose(stderr);
		}

		execve(config->command, argv, envp);
		/* uh-oh, exec failed (bad binary? non-executable?
		   who knows!), and we can't error because we just
		   redirected standard error to /dev/null. -_- */
		exit(42);

	} else {
		fprintf(stderr, "pid %d `%s`\n", config->pid, config->command);
	}
}

static struct child *CONFIG;

void reaper(int sig, siginfo_t *info, void *_)
{
	struct child *chain = CONFIG;
	while (chain) {
		if (chain->pid == info->si_pid) {
			int rc;
			waitpid(chain->pid, &rc, 0);
			chain->pid = 0;
			break;
		}
		chain = chain->next; /* ooh! linked list! */
	}
}

#define PROGRAM "init"

#define INITTAB "/etc/inittab"

int main(int argc, char **argv)
{
	struct sigaction sa;
	struct timespec nap;
	int rc;

	if (argc == 1) {
		CONFIG = configure(INITTAB);

	} else if (argc == 2) {
		if (eq(argv[1], "-v")) show_version(PROGRAM);
		CONFIG = configure(argv[1]);

	} else {
		fprintf(stderr, "USAGE: " PROGRAM " [" INITTAB "]\n");
		exit(EXIT_IMPROPER);
	}


	if (!CONFIG)
		exit(EXIT_IMPROPER);

	sa.sa_sigaction = reaper;
	sa.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
	rc = sigaction(SIGCHLD, &sa, NULL);
	if (rc != 0) {
		fprintf(stderr, "failed to set up SIGCLD handler: %s\n", strerror(errno));
		exit(EXIT_RUNTIME);
	}

	nap.tv_sec = 0;
	nap.tv_nsec = 100000000;
	for (;;) {
		struct child *tmp;
		tmp = CONFIG;
		while (tmp) {
			if (tmp->pid == 0 || kill(tmp->pid, 0) != 0)
				spin(tmp);
			tmp = tmp->next;
		}

		while (nanosleep(&nap, NULL) == -1);

		if (nap.tv_sec == 0) {
			nap.tv_nsec += 100000000;
			if (nap.tv_nsec >= 1000000000) {
				nap.tv_sec = 1;
				nap.tv_nsec %= 1000000000;
			}

		} else if (nap.tv_sec < 10) {
			nap.tv_nsec = 0;
			nap.tv_sec++;
		}
	}

	exit(EXIT_OK);
}
