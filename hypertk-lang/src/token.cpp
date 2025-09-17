#include <string>

#include "token.hpp"

namespace token
{
    Token::Token(TokenType type, int line) : type{type}, line{line} {}

    Token::Token(TokenType type, const std::string &lexeme, int line)
        : type{type}, lexeme{lexeme}, line{line} {}

    /** @brief move constructor */
    Token::Token(Token &&other) noexcept
        : type(other.type), lexeme(std::move(other.lexeme)), line(other.line) {}

    Token &Token::operator=(Token &&other) noexcept
    {
        if (this != &other)
        {
            type = other.type;
            lexeme = std::move(other.lexeme);
            line = other.line;
        }

        return *this;
    }
}