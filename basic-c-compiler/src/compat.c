#include <string.h>
#include <stdlib.h>

#include "compat.h"

#if !_HAVE_STRDUP_
/// @brief An implementation of [strdup](https://en.cppreference.com/w/c/experimental/dynamic/strdup).
/// For C23 and above, please use strdup from string.h
char *strdup(const char *c)
{
    size_t len_mem = strlen(c) + 1;
    char *dup = (char *)malloc(len_mem);
    if (dup != NULL)
    {
        memcpy(dup, c, len_mem);
    }

    return dup;
}
#endif