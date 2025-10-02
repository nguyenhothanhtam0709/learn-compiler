#ifndef C_DATA_H
#define C_DATA_H

#include <stdio.h>

#include "defs.h"

#ifndef extern_
#define extern_ extern
#endif

extern_ int Line;                       // Current line number
extern_ int Putback;                    // Character put back by scanner
extern_ FILE *Infile;                   // Input file
extern_ FILE *Outfile;                  // Output file
extern_ struct token Token;             // Last token scanned
extern_ char Text[TEXTLEN + 1];         // Last identifier scanned
extern_ struct symtable Gsym[NSYMBOLS]; // Global symbol table

#endif