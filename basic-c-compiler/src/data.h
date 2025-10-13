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
/// @brief Name of file we opened as Outfile
extern_ char *Outfilename;
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

/// @brief If true, dump the AST trees
extern_ int O_dumpAST;
/// @brief If true, keep any assembly files
extern_ int O_keepasm;
/// @brief If true, assemble the assembly files
extern_ int O_assemble;
/// @brief If true, link the object files
extern_ int O_dolink;
/// @brief If true, print info on compilation stages
extern_ int O_verbose;

#endif