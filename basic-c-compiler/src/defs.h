#ifndef C_DEFS_H
#define C_DEFS_H

/// @brief Length of symbols in input
#define TEXTLEN 512
/// @brief Number of symbol table entries
#define NSYMBOLS 1024

// Commands and default filenames
#define AOUT "a.out"
#if defined(__APPLE__) && defined(__MACH__)
#define ASCMD "/usr/bin/as -o "
#define LDCMD "/usr/bin/clang -o "
#else
#define ASCMD "as -o "
#define LDCMD "gcc --no-pie -o "
#endif

/// @brief Tokens
enum
{
    T_EOF,
    // #region Binary operators
    T_ASSIGN, // `=`
    T_LOGOR,  // `||`
    T_LOGAND, // `&&`
    T_OR,     // `|`
    T_XOR,    // `^`
    T_AMPER,
    T_EQ,     // `==`
    T_NE,     // `!=`
    T_LT,     // `<`
    T_GT,     // `>`
    T_LE,     // `<=`
    T_GE,     // `>=`
    T_LSHIFT, // `<<`
    T_RSHIFT, // `>>`
    T_PLUS,   // `+`
    T_MINUS,  // `-`
    T_STAR,   // `*`
    T_SLASH,  // `/`
    // #endregion
    // #region Other operatos
    T_INC,    // `++`
    T_DEC,    // `--`
    T_INVERT, // `~`
    T_LOGNOT, // `!`
    // #endregion
    // #region Keywords
    T_VOID, // `void`
    T_CHAR, // `char`
    T_INT,  // `int`
    T_LONG, // `long`
    // #endregion
    // #region Other keywords
    T_IF,     // `if`
    T_ELSE,   // `else`
    T_WHILE,  // `while`
    T_FOR,    // `for`
    T_RETURN, // `return`
    // #endregion
    // #region Structural tokens
    T_INTLIT, // Integer literal
    T_STRLIT, // String literal
    T_SEMI,   // `;`
    T_IDENT,
    T_LBRACE,
    T_RBRACE,
    T_LPAREN,
    T_RPAREN,
    T_LBRACKET,
    T_RBRACKET,
    T_COMMA,
    T_ELLIPSIS, // `...`
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
    A_ASSIGN = 1,
    A_LOGOR,
    A_LOGAND,
    A_OR,
    A_XOR,
    A_AND,
    A_EQ,
    A_NE,
    A_LT,
    A_GT,
    A_LE,
    A_GE,
    A_LSHIFT,
    A_RSHIFT,
    A_ADD,
    A_SUBTRACT,
    A_MULTIPLY,
    A_DIVIDE,
    A_INTLIT, // Integer literal
    A_STRLIT, // String literal
    A_IDENT,
    A_GLUE,
    A_IF,
    A_WHILE,
    A_FUNCTION,
    A_WIDEN, // widen the more narrow child's value to be as wide as the wider child's value
    A_RETURN,
    A_FUNCCALL,
    A_DEREF, // Dereference the pointer in the child node
    A_ADDR,  // Get the address of the identifier in this node
    A_SCALE,
    A_PREINC,
    A_PREDEC,
    A_POSTINC,
    A_POSTDEC,
    A_NEGATE,
    A_INVERT,
    A_LOGNOT,
    A_TOBOOL
};

/// @brief Primitive type. The bottom 4 bits is an integer
/// value that represents the level of indirection,
/// e.g. 0= no pointer, 1= pointer, 2= pointer pointer etc.
enum
{
    P_NONE, // Indicate that the AST node doesn't represent an expression and have no type;
    P_VOID = 16,
    P_CHAR = 32,
    P_INT = 48,
    P_LONG = 64,
};

/// @note
/// - For A_ASSIGN,`right` is actually lvalue
struct ASTnode
{
    /// @brief "Operation" to be performed on this tree.
    int op;
    /// @brief Type of any expression this tree generates.
    int type;
    /// @brief True if the node is an rvalue
    /// @note Before we can determine a specific node is rvalue, all nodes are assumed as lvalue by the compiler.
    int rvalue;
    struct ASTnode *left;
    struct ASTnode *mid;
    struct ASTnode *right;
    /// @brief For A_INTLIT, the integer value.
    /// For A_IDENT, the symbol slot number.
    /// For A_FUNCTION, the symbol slot number.
    /// For A_SCALE, the size to scale by
    /// For A_FUNCCALL, the symbol slot number.
    union
    {
        int intvalue;
        int id;
        int size;
    };
};

/// @brief Use NOREG when the AST generation
/// functions have no register to return
#define NOREG -1
/// @brief Use NOLABEL when we have no label to
/// pass to genAST()
#define NOLABEL 0

/// @brief Structural types
enum
{
    S_VARIABLE,
    S_FUNCTION,
    S_ARRAY
};

/// @brief Storage classes
enum
{
    C_GLOBAL = 1, // Globally visible symbol
    C_LOCAL,      // Locally visible symbol
    C_PARAM,      // Locally visible function parameter
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
    /// @brief Storage class for the symbol
    int class;
    union
    {
        /// @brief Number of elements in the symbol
        int size;
        /// @brief For S_FUNCTIONs, the end label
        int endlabel;
    };
    union
    {
        /// @brief For locals, either the negative offset from the stack base pointer or register id
        int posn;
        /// @brief For functions, number of params. For variadic functions, number of fixed params.
        /// For structs, number of fields
        int nelems;
    };
    /// @brief For S_FUNCTIONs, indicating this function is implemented or not
    int isimplemented;
    /// @brief For S_FUNCTIONs, indicating this function is variadic function or not
    int is_variadic;
};

#endif