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
    T_EQ,     // `==`
    T_NE,     // `!=`
    T_LT,     // `<`
    T_GT,     // `>`
    T_LE,     // `<=`
    T_GE,     // `>=`
    T_INTLIT, // Integer literal
    T_SEMI,   // `;`
    T_ASSIGN, // `=`
    T_IDENT,
    T_LBRACE,
    T_RBRACE,
    T_LPAREN,
    T_RPAREN,
    // #region Keywords
    T_PRINT, // `print`
    T_INT,   // `int`
    T_IF,    // `if`
    T_ELSE,  // `else`
    T_WHILE, // `while`
    T_FOR,   // `for`
    T_VOID,  // `void`
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
    A_ADD = 1,
    A_SUBTRACT,
    A_MULTIPLY,
    A_DIVIDE,
    A_EQ,
    A_NE,
    A_LT,
    A_GT,
    A_LE,
    A_GE,
    A_INTLIT, // Integer literal
    A_IDENT,
    A_LVIDENT, // lvalue identifier
    A_ASSIGN,
    A_PRINT,
    A_GLUE,
    A_IF,
    A_WHILE,
    A_FUNCTION,
};

struct ASTnode
{
    int op; // "Operation" to be performed on this tree
    struct ASTnode *left;
    struct ASTnode *mid;
    struct ASTnode *right;
    union
    {
        int intvalue; // For A_INTLIT, the integer value
        int id;       // For A_IDENT, the symbol slot number
    } v;              // For A_FUNCTION, the symbol slot number
};

/// @brief Use NOREG when the AST generation
/// functions have no register to return
#define NOREG -1

/// @brief Symbol table entry structure
struct symtable
{
    /// @brief Name of a symbol
    char *name;
};

#endif