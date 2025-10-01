#include <stdlib.h>

#include "defs.h"
#include "data.h"
#include "decl.h"

/// @brief Build and return a generic AST node
struct ASTnode *mkastnode(int op,
                          struct ASTnode *left,
                          struct ASTnode *right,
                          int intvalue)
{
    struct ASTnode *n = (struct ASTnode *)malloc(sizeof(struct ASTnode));
    if (n == NULL)
    {
        fprintf(stderr, "Unable to malloc in mkastnode()\n");
        exit(EXIT_FAILURE);
    }

    n->op = op;
    n->left = left;
    n->right = right;
    n->intvalue = intvalue;
    return n;
}

/// @brief Make an AST leaf node
struct ASTnode *mkastleaf(int op, int intvalue)
{
    return mkastnode(op, NULL, NULL, intvalue);
}

/// @brief Make a unary AST node: only one child
struct ASTnode *mkastunary(int op, struct ASTnode *left, int intvalue)
{
    return mkastnode(op, left, NULL, intvalue);
}