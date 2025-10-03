///
/// Parsing statements
///

#include "defs.h"
#include "data.h"
#include "decl.h"

static struct ASTnode *print_statement(void)
{
    struct ASTnode *tree;
    int reg;

    // Match a 'print' as the first token
    match(T_PRINT, "print");

    // Parse the following expression
    tree = binexpr(0);

    // Make an print AST tree
    tree = mkastunary(A_PRINT, tree, 0);

    // Match the following semicolon
    semi();

    return tree;
}

static struct ASTnode *assignment_statement(void)
{
    struct ASTnode *left, *right, *tree;
    int id;

    // Ensure we have an identifier
    ident();

    // Check it's been defined then make a leaf node for it
    if ((id = findglob(Text)) == -1)
        fatals("Undeclared variable", Text);
    right = mkastleaf(A_LVIDENT, id);

    // Ensure we have an equals sign
    match(T_ASSIGN, "=");

    // Parse the follow expression
    left = binexpr(0);

    // Make an assignment AST tree
    tree = mkastnode(A_ASSIGN, left, NULL, right, 0);

    // Match the following semicolon
    semi();

    return tree;
}

/// @brief Parse an IF statement including
/// any optional ELSE clause
/// and return its AST
struct ASTnode *if_statement(void)
{
    struct ASTnode *condAST, *trueAST, *falseAST = NULL;

    // Ensure we have 'if' '('
    match(T_IF, "if");
    lparen();

    // Parse the following expression
    // and the ')' following. Ensure
    // the tree's operation is a comparison.
    condAST = binexpr(0);

    /// @note Only support for A_EQ, A_NE, A_LT, A_GT, A_LE and A_GE
    if (condAST->op < A_EQ || condAST->op > A_GE)
        fatal("Bad comparison operator");
    rparen();

    // Get the AST for the compound statement
    trueAST = compound_statement();

    // If we have an 'else', skip it
    // and get the AST for the compound statement
    if (T_ELSE == Token.token)
    {
        scan(&Token);
        falseAST = compound_statement();
    }

    // Build and return the AST for this statement
    return mkastnode(A_IF, condAST, trueAST, falseAST, 0);
}

/// @brief Parse a compound statement
/// and return its AST
struct ASTnode *compound_statement(void)
{
    struct ASTnode *left = NULL;
    struct ASTnode *tree = NULL;

    // Require a left curly bracket
    lbrace();

    for (;;)
    {
        switch (Token.token)
        {
        case T_PRINT:
            tree = print_statement();
            break;
        case T_INT:
            var_declaration();
            tree = NULL; // No AST generated here
            break;
        case T_IDENT:
            tree = assignment_statement();
            break;
        case T_IF:
            tree = if_statement();
            break;
        case T_RBRACE:
            // When we hit a right curly bracket,
            // skip past it and return the AST
            rbrace();
            return left;
        default:
            fatald("Syntax error, token", Token.token);
        }

        // For each new tree, either save it in left
        // if left is empty, or glue the left and the
        // new tree together
        if (tree)
        {
            if (left == NULL)
                left = tree;
            else
                left = mkastnode(A_GLUE, left, NULL, tree, 0);
        }
    }
}