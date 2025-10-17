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
/// @brief Symbol ptr of the current function
extern_ struct symtable *Functionid;
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

// #region Symbol table lists
/// @brief Global variables and functions
extern_ struct symtable *Globhead, *Globtail;
/// @brief Local variables
extern_ struct symtable *Loclhead, *Locltail;
/// @brief Local parameters
extern_ struct symtable *Parmhead, *Parmtail;
/// @brief Temp list of struct/union members
extern_ struct symtable *Membhead, *Membtail;
/// @brief List of struct types
extern_ struct symtable *Structhead, *Structtail;
/// @brief List of union types
extern_ struct symtable *Unionhead, *Uniontail;
/// @brief List of enum types and values
extern_ struct symtable *Enumhead, *Enumtail;
/// @brief List of typedefs
extern_ struct symtable *Typehead, *Typetail;
// #endregion

// #region Command-line flags
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
// #endregion

#endif