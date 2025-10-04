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
    // #region Operators
    T_PLUS,  // `+`
    T_MINUS, // `-`
    T_STAR,  // `*`
    T_SLASH, // `/`
    T_EQ,    // `==`
    T_NE,    // `!=`
    T_LT,    // `<`
    T_GT,    // `>`
    T_LE,    // `<=`
    T_GE,    // `>=`
    // #endregion
    // #region Types
    T_VOID, // `void`
    T_CHAR, // `char`
    T_INT,  // `int`
    T_LONG, // `long`
    // #endregion
    // #region Structural tokens
    T_INTLIT, // Integer literal
    T_SEMI,   // `;`
    T_ASSIGN, // `=`
    T_IDENT,
    T_LBRACE,
    T_RBRACE,
    T_LPAREN,
    T_RPAREN,
    // #endregion
    // #region Other keywords
    T_PRINT,  // `print`
    T_IF,     // `if`
    T_ELSE,   // `else`
    T_WHILE,  // `while`
    T_FOR,    // `for`
    T_RETURN, // `return`
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
    A_WIDEN, // widen the more narrow child's value to be as wide as the wider child's value
    A_RETURN,
    A_FUNCCALL,
};

/// @brief Primitive type
enum
{
    P_NONE, // Indicate that the AST node doesn't represent an expression and have no type;
    P_VOID,
    P_CHAR,
    P_INT,
    P_LONG,
};

struct ASTnode
{
    /// @brief "Operation" to be performed on this tree.
    int op;
    /// @brief Type of any expression this tree generates.
    int type;
    struct ASTnode *left;
    struct ASTnode *mid;
    struct ASTnode *right;
    /// @brief For A_INTLIT, the integer value.
    /// For A_IDENT, the symbol slot number.
    /// For A_FUNCTION, the symbol slot number.
    /// For A_FUNCCALL, the symbol slot number.
    union
    {
        int intvalue;
        int id;
    } v;
};

/// @brief Use NOREG when the AST generation
/// functions have no register to return
#define NOREG -1

/// @brief Structural types
enum
{
    S_VARIABLE,
    S_FUNCTION
};

/// @brief Symbol table entry structure
struct symtable
{
    /// @brief Name of a symbol
    char *name;
    /// @brief Primitive type for the symbol
    int type;
    /// @brief Structural type for the symbol
    int stype;
    /// @brief For S_FUNCTIONs, the end label
    int endlabel;
};

#endif