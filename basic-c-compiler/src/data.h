#ifndef C_DATA_H
#define C_DATA_H

#include <stdio.h>

#include "defs.h"

#ifndef extern_
#define extern_ extern
#endif

/// @brief Current line number
extern_ int Line;
/// @brief Character put back by scanner
extern_ int Putback;
/// @brief Symbol id of the current function
extern_ int Functionid;
/// @brief Position of next free global symbol slot
extern_ int Globs;
/// @brief Position of next free local symbol slot
extern_ int Locls;
/// @brief Input file
extern_ FILE *Infile;
/// @brief Output file
extern_ FILE *Outfile;
/// @brief token scanned
extern_ struct token Token;
/// @brief Last identifier scanned
extern_ char Text[TEXTLEN + 1];
/// @brief Global symbol table
///
/// @note Symbol table layout:
/// 0xxxx......................................xxxxxxxxxxxxNSYMBOLS-1
///     ^                                    ^
///     |                                    |
///   Globs                                Locls
extern_ struct symtable Symtable[NSYMBOLS];

extern_ int O_dumpAST;

#endif