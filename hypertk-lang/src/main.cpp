#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>

#include "lexer.hpp"
#include "token.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "ast_printer.hpp"
#include "runtime_llvm.hpp"

int main()
{
    std::string src = "func foo(a,b) { a*a + 2*a*b + b*b; return a*a + 2*a*b;}";
    parser::Parser parser_{lexer::Lexer{src}};

    auto ast_ = parser_.parse();
    if (ast_.has_value())
    {
        // ast::SimplePrinter printer;
        // printer.print(ast_.value());

        hypertk::RuntimeLLVM runtime;
        runtime.initializeModuleAndManagers();
        runtime.printIR(ast_.value());
    }

    // lexer::Lexer l{src};
    // token::Token token = l.nextToken();
    // while (token.type != token::TokenType::END_OF_FILE && token.type != token::TokenType::ERROR)
    // {
    //     std::cout << "Current token: " << token.lexeme << "\n";
    //     token = l.nextToken();
    // }

    // std::cout << "Current token: " << src.at(src.length() - 1) << "\n";

    // std::unique_ptr<ast::statement::Expression> exprStmt = std::make_unique<ast::statement::Expression>(
    //     std::make_unique<ast::expression::Number>(10));

    // AstVisitor visitor;
    // visitor.visit(std::move(exprStmt));

    return EXIT_SUCCESS;
}