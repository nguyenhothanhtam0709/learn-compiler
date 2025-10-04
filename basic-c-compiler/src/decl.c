///
/// Parse declarations
///

#include "defs.h"
#include "data.h"
#include "decl.h"

/// @brief Parse the current token and
/// return a primitive type enum value
int parse_type(int t)
{
    if (t == T_CHAR)
        return P_CHAR;
    if (t == T_INT)
        return P_INT;
    if (t == T_LONG)
        return P_LONG;
    if (t == T_VOID)
        return P_VOID;

    fatald("Illegal type, token", t);
}

/// @brief Parse the declaration of a variable
void var_declaration(void)
{
    int id, type;

    // Get the type of the variable, then the identifier
    type = parse_type(Token.token);
    scan(&Token);
    ident();
    id = addglob(Text, type, S_VARIABLE, 0);
    genglobsym(id);
    semi();
}

/// @brief For now we have a very simplistic function definition grammar
///
/// function_declaration: 'void' identifier '(' ')' compound_statement   ;
///
/// Parse the declaration of a simplistic function
struct ASTnode *function_declaration(void)
{
    struct ASTnode *tree, *finalstmt;
    int nameslot, type, endlabel;

    // Get the type of the variable, then the identifier
    type = parse_type(Token.token);
    scan(&Token);
    ident();

    // Get a label-id for the end label, add the function
    // to the symbol table, and set the Functionid global
    // to the function's symbol-id
    endlabel = genlabel();
    nameslot = addglob(Text, type, S_FUNCTION, endlabel);
    Functionid = nameslot;

    // Scan in the parentheses
    lparen();
    rparen();

    // Get the AST tree for the compound statement
    tree = compound_statement();

    if (type != P_VOID)
    {
        finalstmt = (tree->op == A_GLUE) ? tree->right : tree;
        if (finalstmt == NULL || finalstmt->op != A_RETURN)
            fatal("No return for function with non-void type");
    }
    
    // Return an A_FUNCTION node which has the function's nameslot
    // and the compound statement sub-tree
    return mkastunary(A_FUNCTION, type, tree, nameslot);
}