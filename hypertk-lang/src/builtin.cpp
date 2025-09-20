#include <cstdio>

#include "builtin.hpp"

double putchard(double X)
{
    fputc((char)X, stderr);
    return 0;
}

double printd(double X)
{
    fprintf(stderr, "%f\n", X);
    return 0;
}
