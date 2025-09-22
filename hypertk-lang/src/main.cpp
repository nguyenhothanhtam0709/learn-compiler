#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>

#include "common.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "ast_printer.hpp"
#include "runtime_llvm.hpp"
#include "builtin.hpp"
#include "error.hpp"

int main()
{
    std::string src = R"(
        // func unary!(v) {
        //     if (v) return 0;
        //     else return 1;
        // }

        func binary> 10 (LHS, RHS) {
            return RHS < LHS;
        }

        func unary-(v) {
            return 0 - v;
        }

        func foo(a,b) { 
            // a*a + 2*a*b + b*b;
            return a*a + 2*a*b;
        }

        func printstar(n) {
            for i = 1, i < n, 1.0 in
                printd(42);
        }

        func main() {
            // if (0) printd(3);

            // if (1)
            //     printd(1);
            // else 
            //     1*1;
            
            // printd(foo(1, 2) ? 1 : 2);

            // printstar(3);

            return -1;
        }
    )";
    parser::Parser parser_{lexer::Lexer{src}};

    auto ast_ = parser_.parse();
    if (error::hasError())
        return EXIT_FAILURE;
    if (ast_.has_value())
    {
        // ast::SimplePrinter printer;
        // printer.print(ast_.value());

        hypertk::RuntimeLLVM runtime;
#ifdef ENABLE_BASIC_JIT_COMPILER
        runtime.initializeJIT();
#endif
        runtime.initializeModuleAndManagers();
#ifdef ENABLE_BUILTIN_FUNCTIONS
        runtime.declareBuiltInFunctions();
#endif
        runtime.genIR(ast_.value());
        if (error::hasError())
            return EXIT_FAILURE;

        runtime.printIR();

#ifdef ENABLE_BASIC_JIT_COMPILER
        runtime.eval();
#endif
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