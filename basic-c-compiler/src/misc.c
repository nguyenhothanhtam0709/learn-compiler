#include <stdlib.h>
#include <unistd.h>

#include "defs.h"
#include "data.h"
#include "decl.h"

/// @brief Ensure that the current token is t,
/// and fetch the next token. Otherwise
/// throw an error
void match(int t, char *what)
{
    if (Token.token == t)
    {
        scan(&Token);
    }
    else
    {
        fatals("Expected", what);
    }
}

/// @brief Match a semicon and fetch the next token
void semi(void)
{
    match(T_SEMI, ";");
}

/// @brief Match a left brace and fetch the next token
void lbrace(void)
{
    match(T_LBRACE, "{");
}

/// @brief Match a right brace and fetch the next token
void rbrace(void)
{
    match(T_RBRACE, "}");
}

/// @brief Match a left parenthesis and fetch the next token
void lparen(void)
{
    match(T_LPAREN, "(");
}

/// @brief Match a right parenthesis and fetch the next token
void rparen(void)
{
    match(T_RPAREN, ")");
}

/// @brief Match an identifier and fetch the next token
void ident(void)
{
    match(T_IDENT, "identifier");
}

/// @brief Match a comma and fetch the next token
void comma(void)
{
    match(T_COMMA, "comma");
}

/// @brief Print out fatal messages
void fatal(char *s)
{
    fprintf(stderr, "%s on line %d of %s\n", s, Line, Infilename);
    fclose(Outfile);
    unlink(Outfilename);
    exit(EXIT_FAILURE);
}

void fatals(char *s1, char *s2)
{
    fprintf(stderr, "%s:%s on line %d of %s\n", s1, s2, Line, Infilename);
    fclose(Outfile);
    unlink(Outfilename);
    exit(EXIT_FAILURE);
}

void fatald(char *s, int d)
{
    fprintf(stderr, "%s:%d on line %d of %s\n", s, d, Line, Infilename);
    fclose(Outfile);
    unlink(Outfilename);
    exit(EXIT_FAILURE);
}

void fatalc(char *s, int c)
{
    fprintf(stderr, "%s:%c on line %d of %s\n", s, c, Line, Infilename);
    fclose(Outfile);
    unlink(Outfilename);
    exit(EXIT_FAILURE);
}
