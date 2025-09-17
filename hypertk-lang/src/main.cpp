#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>

#include "lexer.hpp"
#include "token.hpp"
#include "ast.hpp"

class AstVisitor : public ast::expr::Visitor<void>, public ast::stmt::Visitor<void>
{
public:
    using ast::expr::Visitor<void>::visit;
    using ast::stmt::Visitor<void>::visit;

protected:
    void visitNumberExpr(const ast::expr::Number &expr) override
    {
        std::cout << "Visit number expression " << expr.Val << '\n';
    }
    void visitVariableExpr(const ast::expr::Variable &expr) override {}
    void visitBinaryExpr(const ast::expr::Binary &expr) override {}
    void visitCallExpr(const ast::expr::Call &expr) override {}

    void visitFunctionStmt(const ast::stmt::Function &stmt) override {}
    void visitExpressionStmt(const ast::stmt::Expression &stmt) override
    {
        std::cout << "visit expression statement\n";
        visit(stmt.Expr);
    }
};

int main()
{
    // std::string src = "func foo() {}";
    // hypertk::Lexer lexer("func foo() {}");
    // hypertk::Token token = lexer.nextToken();
    // while (token.type != hypertk::TokenType::END_OF_FILE && token.type != hypertk::TokenType::ERROR)
    // {
    //     std::cout << "Current token: " << token.lexeme << "\n";
    //     token = lexer.nextToken();
    // }

    // std::cout << "Current token: " << src.at(src.length() - 1) << "\n";

    std::unique_ptr<ast::stmt::Expression> exprStmt = std::make_unique<ast::stmt::Expression>(
        std::make_unique<ast::expr::Number>(10));

    AstVisitor visitor;
    visitor.visit(std::move(exprStmt));

    return EXIT_SUCCESS;
}