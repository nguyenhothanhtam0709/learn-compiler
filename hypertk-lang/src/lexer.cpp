#include <string>

#include "lexer.hpp"
#include "token.hpp"

namespace lexer
{
    Lexer::Lexer(const std::string &src)
        : src_{src}, start_{0}, current_{0}, line_{1} {}

    Lexer::Lexer(Lexer &&other)
        : src_{std::move(other.src_)}, start_{other.start_}, current_{other.current_}, line_{other.line_} {}
    Lexer &Lexer::operator=(Lexer &&other)
    {
        if (this != &other)
        {
            src_ = std::move(other.src_);
            start_ = other.start_;
            current_ = other.current_;
            line_ = other.line_;
        }

        return *this;
    }

    token::Token Lexer::nextToken()
    {
        skipWhitespaceAndComment();

        start_ = current_;

        if (isAtEnd())
            return token::Token(token::TokenType::END_OF_FILE, line_);

        char c = advance();

        if (isalpha(c))
            return identifier();
        if (isdigit(c))
            return number();

        if (c == '+')
            return makeToken(token::TokenType::PLUS);
        if (c == '-')
            return makeToken(token::TokenType::MINUS);
        if (c == '*')
            return makeToken(token::TokenType::STAR);
        if (c == '/')
            return makeToken(token::TokenType::SLASH);
        if (c == '=')
            return makeToken(token::TokenType::EQUAL);
        if (c == '<')
            return makeToken(token::TokenType::LESS);
        if (c == '>')
            return makeToken(token::TokenType::GREATER);
        if (c == '!')
            return makeToken(token::TokenType::EXCLAMATION);
        if (c == '(')
            return makeToken(token::TokenType::LEFT_PAREN);
        if (c == ')')
            return makeToken(token::TokenType::RIGHT_PAREN);
        if (c == '{')
            return makeToken(token::TokenType::LEFT_BRACE);
        if (c == '}')
            return makeToken(token::TokenType::RIGHT_BRACE);
        if (c == '?')
            return makeToken(token::TokenType::QUESTION_MARK);
        if (c == ':')
            return makeToken(token::TokenType::COLON);
        if (c == ';')
            return makeToken(token::TokenType::SEMICOLON);
        if (c == ',')
            return makeToken(token::TokenType::COMMA);
        if (c == '|')
            return makeToken(token::TokenType::VERTICAL_BAR);
        if (c == '&')
            return makeToken(token::TokenType::AMPERSAND);

        return errorToken(std::string("Unexpected character '") + c + "'.");
    }

    token::Token Lexer::number()
    {
        while (isdigit(peek()))
            advance();

        if (peek() == '.' && isdigit(peekNext()))
        {
            advance(); // consume `.`

            while (isdigit(peek()))
                advance();
        }

        return makeToken(token::TokenType::NUMBER);
    }
    token::Token Lexer::identifier()
    {
        while (isalnum(peek()))
            advance();

        const auto lexeme = makeLexeme();
        token::TokenType type;
        if (lexeme == "func")
            type = token::TokenType::FUNC;
        else if (lexeme == "return")
            type = token::TokenType::RETURN;
        else if (lexeme == "if")
            type = token::TokenType::IF;
        else if (lexeme == "then")
            type = token::TokenType::THEN;
        else if (lexeme == "else")
            type = token::TokenType::ELSE;
        else if (lexeme == "for")
            type = token::TokenType::FOR;
        else if (lexeme == "in")
            type = token::TokenType::IN;
        else if (lexeme == "unary")
            type = token::TokenType::UNARY;
        else if (lexeme == "binary")
            type = token::TokenType::BINARY;
        else
            type = token::TokenType::IDENTIFIER;

        return makeToken(type, lexeme);
    }

    void Lexer::skipWhitespaceAndComment() noexcept
    {
        while (true)
        {
            char c = peek();
            switch (c)
            {
            case ' ':
            case '\r':
            case '\t':
            {
                advance();
                break;
            }
            case '\n':
            {
                line_++;
                advance();
                break;
            }
            case '/':
            {
                if (peekNext() == '/')
                {
                    while (peek() != '\n' && !isAtEnd())
                    {
                        advance();
                    }
                }
                else
                {
                    return;
                }
                break;
            }
            default:
                return;
            }
        }
    }

    inline token::Token Lexer::makeToken(token::TokenType type) { return token::Token(type, makeLexeme(), line_); }
    inline token::Token Lexer::makeToken(token::TokenType type, const std::string &lexeme) { return token::Token(type, lexeme, line_); }
    inline std::string Lexer::makeLexeme() { return src_.substr(start_, current_ - start_); }
    inline token::Token Lexer::errorToken(const std::string &msg) { return makeToken(token::TokenType::ERROR, msg); }

    bool Lexer::match(char expected) noexcept
    {
        if (isAtEnd())
            return false;

        if (peek() != expected)
            return false;

        current_++;
        return true;
    }
    inline char Lexer::advance() noexcept { return isAtEnd() ? '\0' : src_.at(current_++); }
    inline char Lexer::peek() const noexcept { return isAtEnd() ? '\0' : src_.at(current_); }
    inline char Lexer::peekNext() const noexcept { return nextIsEnd() ? '\0' : src_.at(current_ + 1); }
    inline bool Lexer::isAtEnd() const noexcept { return current_ >= src_.length(); }
    inline bool Lexer::nextIsEnd() const noexcept { return current_ + 1 >= src_.length(); }
}
