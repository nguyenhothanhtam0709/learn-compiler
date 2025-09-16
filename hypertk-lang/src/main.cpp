#include <cstdlib>
#include <iostream>
#include <string>

#include "lexer.hpp"
#include "token.hpp"

int main()
{
    std::string src = "func foo() {}";
    hypertk::Lexer lexer("func foo() {}");
    hypertk::Token token = lexer.nextToken();
    while (token.type != hypertk::TokenType::END_OF_FILE && token.type != hypertk::TokenType::ERROR)
    {
        std::cout << "Current token: " << token.lexeme << "\n";
        token = lexer.nextToken();
    }

    std::cout << "Current token: " << src.at(src.length() - 1) << "\n";

    return EXIT_SUCCESS;
}