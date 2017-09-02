#ifndef RIG_H
#define RIG_H

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE   500

#define VERSION "1.0"

#define EXIT_OK 0
#define EXIT_IMPROPER 1
#define EXIT_RUNTIME  2
#define EXIT_IN_CHILD 251

#include <string.h>

void show_version(const char *bin);

#define eq(s1,s2) (strcmp((s1), (s2)) == 0)

#endif
