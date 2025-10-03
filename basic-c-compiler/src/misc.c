#include <stdlib.h>

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
        printf("%s expected on line %d\n", what, Line);
        exit(EXIT_FAILURE);
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

/// @brief Print out fatal messages
void fatal(char *s)
{
    fprintf(stderr, "%s on line %d\n", s, Line);
    exit(EXIT_FAILURE);
}

void fatals(char *s1, char *s2)
{
    fprintf(stderr, "%s:%s on line %d\n", s1, s2, Line);
    exit(EXIT_FAILURE);
}

void fatald(char *s, int d)
{
    fprintf(stderr, "%s:%d on line %d\n", s, d, Line);
    exit(EXIT_FAILURE);
}

void fatalc(char *s, int c)
{
    fprintf(stderr, "%s:%c on line %d\n", s, c, Line);
    exit(EXIT_FAILURE);
}
