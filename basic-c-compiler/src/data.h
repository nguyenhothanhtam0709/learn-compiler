#ifndef C_DATA_H
#define C_DATA_H

#include <stdio.h>

#include "defs.h"

#ifndef extern_
#define extern_ extern
#endif

extern_ int Line;
extern_ int Putback;
extern_ FILE *Infile;
extern_ FILE *Outfile;
extern_ struct token Token;

#endif