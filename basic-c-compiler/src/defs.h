#ifndef C_DEFS_H
#define C_DEFS_H

/// @brief Tokens
enum
{
    T_EOF,
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

/// @brief AST node types
enum
{
    A_ADD,
    A_SUBTRACT,
    A_MULTIPLY,
    A_DIVIDE,
    A_INTLIT, // Integer literal
};

struct ASTnode
{
    int op; // "Operation" to be performed on this tree
    struct ASTnode *left;
    struct ASTnode *right;
    int intvalue; // For A_INTLIT, the integer value
};

#endif