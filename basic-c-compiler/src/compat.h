#ifndef C_COMPAT_H
#define C_COMPAT_H

#include <string.h>

#if !__has_builtin(strdup)
char *strdup(const char *c);
#endif

#ifndef __deprecated
#define __deprecated __attribute__((deprecated))
#endif

#endif