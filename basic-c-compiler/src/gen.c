#include <stdlib.h>

#include "defs.h"
#include "data.h"
#include "decl.h"

/// @brief Given an AST, generate
/// assembly code recursively
static int genAST(struct ASTnode *n)
{
    /// @note `genAST()`'s code passes around register identifiers.
    /// `genAST()` itself returns the identity of the register that holds the final value of the tree at this point.
    ///

    int leftreg,  // Register hold returned value of LHS
        rightreg; // Register hold returned value of RHS

    // Get the left and right sub-tree values
    if (n->left)
        leftreg = genAST(n->left);
    if (n->right)
        rightreg = genAST(n->right);

    switch (n->op)
    {
    case A_ADD:
        return cgadd(leftreg, rightreg);
    case A_SUBTRACT:
        return cgsub(leftreg, rightreg);
    case A_MULTIPLY:
        return cgmul(leftreg, rightreg);
    case A_DIVIDE:
        return cgdiv(leftreg, rightreg);
    case A_INTLIT:
        return cgload(n->intvalue);
    default:
        fprintf(stderr, "Unknown AST operator %d\n", n->op);
        exit(EXIT_FAILURE);
    }
}

void generatecode(struct ASTnode *n)
{
    int reg;

    cgpreamble(); // leading asm code (the preamble)
    reg = genAST(n);
    cgprintint(reg); // Print the register with the result as an int
    cgpostamble();   //  trailing asm code (the postamble)
}