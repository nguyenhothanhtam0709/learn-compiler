#ifndef C_DECL_H
#define C_DECL_H

int scan(struct token *t);

struct ASTnode *mkastnode(int op,
                          struct ASTnode *left,
                          struct ASTnode *right,
                          int intvalue);
struct ASTnode *mkastleaf(int op, int intvalue);
struct ASTnode *mkastunary(int op, struct ASTnode *left, int intvalue);

struct ASTnode *binexpr(int ptp);

void statements(void);

int genAST(struct ASTnode *n);
void genpreamble();
void genpostamble();
void genfreeregs();
void genprintint(int reg);

void freeall_registers(void);
void cgpreamble();
void cgpostamble();
int cgload(int value);
int cgadd(int r1, int r2);
int cgsub(int r1, int r2);
int cgmul(int r1, int r2);
int cgdiv(int r1, int r2);
void cgprintint(int r);

void match(int t, char *what);
void semi(void);

#endif