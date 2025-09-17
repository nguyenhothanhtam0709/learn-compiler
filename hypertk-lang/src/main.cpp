#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>

#include "lexer.hpp"
#include "token.hpp"
#include "ast.hpp"

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

    std::unique_ptr<hypertk::ast::stmt::Expr> exprStmt = std::make_unique<hypertk::ast::stmt::Expr>(
        std::make_unique<hypertk::ast::expr::Expr>(hypertk::ast::expr::Number(100)));
    std::cout << "AA " << exprStmt << "\n";

    return EXIT_SUCCESS;
}