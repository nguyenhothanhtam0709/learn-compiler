#ifndef C_DEFS_H
#define C_DEFS_H

/// @brief Length of symbols in input
#define TEXTLEN 512
/// @brief Number of symbol table entries
#define NSYMBOLS 1024

/// @brief Tokens
enum
{
    T_EOF,
    T_PLUS,   // `+`
    T_MINUS,  // `-`
    T_STAR,   // `*`
    T_SLASH,  // `/`
    T_INTLIT, // Integer literal
    T_SEMI,   // `;`
    T_EQUALS,
    T_IDENT,
    // #region Keywords
    T_PRINT, // `print`
    T_INT,
    // #endregion
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
    A_IDENT,
    A_LVIDENT, // lvalue identifier
    A_ASSIGN
};

struct ASTnode
{
    int op; // "Operation" to be performed on this tree
    struct ASTnode *left;
    struct ASTnode *right;
    union
    {
        int intvalue; // For A_INTLIT, the integer value
        int id;       // For A_IDENT, the symbol slot number
    } v;
};

/// @brief Symbol table entry structure
struct symtable
{
    /// @brief Name of a symbol
    char *name;
};

#endif