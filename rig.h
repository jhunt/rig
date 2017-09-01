#ifndef RIG_H
#define RIG_H

#define VERSION "1.0"

#define EXIT_OK 0
#define EXIT_IMPROPER 1
#define EXIT_RUNTIME  2

#include <string.h>

void show_version(const char *bin);

#define eq(s1,s2) (strcmp((s1), (s2)) == 0)

#endif