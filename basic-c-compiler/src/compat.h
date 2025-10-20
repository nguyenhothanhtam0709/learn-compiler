#ifndef C_COMPAT_H
#define C_COMPAT_H

#include <string.h>

#ifdef _POSIX_C_SOURCE
#if _POSIX_C_SOURCE >= 200809L
#define _HAVE_STRDUP_ 1
#endif
#endif

#ifdef __APPLE__ /* Specific functions of MacOS */
#define _HAVE_STRDUP_ 1
#endif

#if !_HAVE_STRDUP_
char *strdup(const char *c);
#endif

#ifndef __deprecated
#define __deprecated __attribute__((deprecated))
#endif

#endif