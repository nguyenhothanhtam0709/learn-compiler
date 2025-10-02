#include <stdlib.h>

#include "defs.h"
#include "data.h"
#include "decl.h"

/// @brief Given an AST, generate
/// assembly code recursively
int genAST(struct ASTnode *n, int reg)
{
    /// @note `genAST()`'s code passes around register identifiers.
    /// `genAST()` itself returns the identity of the register that holds the final value of the tree at this point.
    ///

    int leftreg,  // Register hold returned value of LHS
        rightreg; // Register hold returned value of RHS

    // Get the left and right sub-tree values
    if (n->left)
        leftreg = genAST(n->left, -1);
    if (n->right)
        rightreg = genAST(n->right, leftreg);

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
    case A_EQ:
        return cgequal(leftreg, rightreg);
    case A_NE:
        return cgnotequal(leftreg, rightreg);
    case A_LT:
        return cglessthan(leftreg, rightreg);
    case A_GT:
        return cggreaterthan(leftreg, rightreg);
    case A_LE:
        return cglessequal(leftreg, rightreg);
    case A_GE:
        return cggreaterequal(leftreg, rightreg);
    case A_INTLIT:
        return cgloadint(n->v.intvalue);
    case A_IDENT:
        return cgloadglob(Gsym[n->v.id].name);
    case A_LVIDENT:
        return cgstorglob(reg, Gsym[n->v.id].name);
    case A_ASSIGN:
        // The work has already been done, return the result
        return rightreg;
    default:
        fprintf(stderr, "Unknown AST operator %d\n", n->op);
        exit(EXIT_FAILURE);
    }
}

void genpreamble()
{
    cgpreamble();
}

void genpostamble()
{
    cgpostamble();
}

void genfreeregs()
{
    freeall_registers();
}

void genprintint(int reg)
{
    cgprintint(reg);
}

void genglobsym(char *s)
{
    cgglobsym(s);
}