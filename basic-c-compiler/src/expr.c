///
/// Parsing of expressions with Pratt parsing
///

#include <stdlib.h>

#include "defs.h"
#include "data.h"
#include "decl.h"

/// @brief Parse a primary factor and return an
/// AST node representing it.
static struct ASTnode *primary(void)
{
    struct ASTnode *n;

    // For an INTLIT token, make a leaf AST node for it
    // and scan in the next token. Otherwise, a syntax error
    // for any other token type.
    switch (Token.token)
    {
    case T_INTLIT:
        n = mkastleaf(A_INTLIT, Token.intvalue);
        scan(&Token);
        return n;
    default:
        fprintf(stderr, "syntax error on line %d\n", Line);
        exit(EXIT_FAILURE);
    }
}

/// @brief Convert a token into an AST operation.
int arithop(int tokentype)
{
    switch (tokentype)
    {
    case T_PLUS:
        return A_ADD;
    case T_MINUS:
        return A_SUBTRACT;
    case T_STAR:
        return A_MULTIPLY;
    case T_SLASH:
        return A_DIVIDE;
    default:
        fprintf(stderr, "unknown token in arithop() on line %d\n", Line);
        exit(EXIT_FAILURE);
    }
}

/// @brief Operator precedence for each token
static int OpPrec[] = {
    [T_EOF] = 0,
    [T_PLUS] = 10,
    [T_MINUS] = 10,
    [T_STAR] = 20,
    [T_SLASH] = 20,
    [T_INTLIT] = 0};

/// @brief Check that we have a binary operator and
/// return its precedence.
static int op_precedence(int tokentype)
{
    int prec = OpPrec[tokentype];
    if (prec == 0)
    {
        fprintf(stderr, "syntax error on line %d, token %d\n", Line, tokentype);
        exit(EXIT_FAILURE);
    }

    return prec;
}

/// @brief Return an AST tree whose root is a binary operator.
/// Parameter ptp is the previous token's precedence.
struct ASTnode *binexpr(int ptp)
{
    struct ASTnode *left, *right;
    int tokentype;

    // Get the integer literal on the left.
    // Fetch the next token at the same time.
    left = primary();

    // If no tokens left, return just the left node
    tokentype = Token.token;
    if (tokentype == T_EOF)
        return left;

    // While the precedence of this token is
    // more than that of the previous token precedence
    while (op_precedence(tokentype) > ptp)
    {
        // Fetch in the next integer literal
        scan(&Token);

        // Recursively call binexpr() with the
        // precedence of our token to build a sub-tree
        right = binexpr(OpPrec[tokentype]);

        // Join that sub-tree with ours. Convert the token
        // into an AST operation at the same time.
        left = mkastnode(arithop(tokentype), left, right, 0);

        // Update the details of the current token.
        // If no tokens left, return just the left node
        tokentype = Token.token;
        if (tokentype == T_EOF)
            return left;
    }

    // Return the tree we have when the precedence
    // is the same or lower
    return left;
}