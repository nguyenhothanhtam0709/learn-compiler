#ifndef C_DEFS_H
#define C_DEFS_H

// Tokens
enum
{
    T_PLUS,   // `+`
    T_MINUS,  // `-`
    T_STAR,   // `*`
    T_SLASH,  // `/`
    T_INTLIT, // Integer literal
};

struct token
{
    int token;
    int intvalue;
};

#endif