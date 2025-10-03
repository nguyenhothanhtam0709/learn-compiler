#ifndef C_COMPAT_H
#define C_COMPAT_H

#include <string.h>

#if !__has_builtin(strdup)
char *strdup(const char *c);
#endif

#endif