///
/// Parsing statements
///

#include "defs.h"
#include "data.h"
#include "decl.h"

static struct ASTnode *single_statement(void);

static struct ASTnode *print_statement(void)
{
    struct ASTnode *tree;

    // Match a 'print' as the first token
    match(T_PRINT, "print");

    // Parse the following expression
    tree = binexpr(0);

    // Make an print AST tree
    tree = mkastunary(A_PRINT, tree, 0);

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

/// @brief Parse a WHILE statement
/// and return its AST
struct ASTnode *while_statement(void)
{
    struct ASTnode *condAST = NULL,
                   *bodyAST = NULL;

    // Ensure we have 'while' '('
    match(T_WHILE, "while");
    lparen();

    // Parse the following expression
    // and the ')' following. Ensure
    // the tree's operation is a comparison.
    condAST = binexpr(0);
    if (condAST->op < A_EQ || condAST->op > A_GE)
        fatal("Bad comparison operator");
    rparen();

    // Get the AST for the compound statement
    bodyAST = compound_statement();

    // Build and return the AST for this statement
    return mkastnode(A_WHILE, condAST, NULL, bodyAST, 0);
}

/// @brief for_statement: 'for' '(' preop_statement ';'
///                          true_false_expression ';'
///                          postop_statement ')' compound_statement  ;
///
/// preop_statement:  statement          (for now)
/// postop_statement: statement          (for now)
///
/// Parse a FOR statement
/// and return its AST
static struct ASTnode *for_statement(void)
{
    struct ASTnode *condAST, *bodyAST;
    struct ASTnode *preopAST, *postopAST;
    struct ASTnode *tree;

    // Ensure we have 'for' '('
    match(T_FOR, "for");
    lparen();

    // Get the pre_op statement and the ';'
    preopAST = single_statement();
    semi();

    // Get the condition and the ';'
    condAST = binexpr(0);
    if (condAST->op < A_EQ || condAST->op > A_GE)
        fatal("Bad comparison operator");
    semi();

    // Get the post_op statement and the ')'
    postopAST = single_statement();
    rparen();

    // Get the compound statement which is the body
    bodyAST = compound_statement();

    // For now, all four sub-trees have to be non-NULL.
    // Later on, we'll change the semantics for when some are missing

    // Glue the compound statement and the postop tree
    tree = mkastnode(A_GLUE, bodyAST, NULL, postopAST, 0);

    // Make a WHILE loop with the condition and this new body
    tree = mkastnode(A_WHILE, condAST, NULL, tree, 0);

    // And glue the preop tree to the A_WHILE tree
    return mkastnode(A_GLUE, preopAST, NULL, tree, 0);
}

/// @brief Parse a single statement
/// and return its AST
static struct ASTnode *single_statement(void)
{
    switch (Token.token)
    {
    case T_PRINT:
        return print_statement();
    case T_INT:
        var_declaration();
        return NULL; // No AST generated here
    case T_IDENT:
        return assignment_statement();
    case T_IF:
        return if_statement();
    case T_WHILE:
        return while_statement();
    case T_FOR:
        return for_statement();
    default:
        fatald("Syntax error, token", Token.token);
    }
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
        // Parse a single statement
        tree = single_statement();

        // Some statements must be followed by a semicolon
        if (tree != NULL && (tree->op == A_PRINT || tree->op == A_ASSIGN))
            semi();

        // For each new tree, either save it in left
        // if left is empty, or glue the left and the
        // new tree together
        if (tree != NULL)
        {
            if (left == NULL)
                left = tree;
            else
                left = mkastnode(A_GLUE, left, NULL, tree, 0);
        }
        // When we hit a right curly bracket,
        // skip past it and return the AST
        if (Token.token == T_RBRACE)
        {
            rbrace();
            return (left);
        }
    }
}
