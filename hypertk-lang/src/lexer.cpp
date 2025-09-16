#include "lexer.hpp"
#include "token.hpp"

namespace hypertk
{
    Lexer::Lexer(const std::string &src)
        : src_{src}, start_{0}, current_{0}, line_{1} {}

    Token Lexer::nextToken()
    {
        skipWhitespaceAndComment();

        start_ = current_;

        if (isAtEnd())
            return Token(TokenType::END_OF_FILE, line_);

        char c = advance();

        if (isalpha(c))
            return identifier();
        if (isdigit(c))
            return number();

        if (c == '+')
            return makeToken(TokenType::PLUS);
        if (c == '-')
            return makeToken(TokenType::MINUS);
        if (c == '*')
            return makeToken(TokenType::STAR);
        if (c == '/')
            return makeToken(TokenType::SLASH);
        if (c == '(')
            return makeToken(TokenType::LEFT_PAREN);
        if (c == ')')
            return makeToken(TokenType::RIGHT_PAREN);
        if (c == '{')
            return makeToken(TokenType::LEFT_BRACE);
        if (c == '}')
            return makeToken(TokenType::RIGHT_BRACE);
        if (c == ':')
            return makeToken(TokenType::COLON);
        if (c == ';')
            return makeToken(TokenType::SEMICOLON);

        return errorToken("Unexpected character.");
    }

    Token Lexer::number()
    {
        while (isdigit(peek()))
            advance();

        if (peek() == '.' && isdigit(peekNext()))
        {
            advance(); // consume `.`

            while (isdigit(peek()))
                advance();
        }

        return makeToken(TokenType::NUMBER);
    }
    Token Lexer::identifier()
    {
        while (isalnum(peek()))
            advance();

        const auto lexeme = makeLexeme();
        TokenType type;
        if (lexeme == "func")
            type = TokenType::FUNC;
        else
            type = TokenType::IDENTIFIER;

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

    inline Token Lexer::makeToken(TokenType type) { return Token(type, makeLexeme(), line_); }
    inline Token Lexer::makeToken(TokenType type, const std::string &lexeme) { return Token(type, lexeme, line_); }
    inline std::string Lexer::makeLexeme() { return src_.substr(start_, current_ - start_); }
    inline Token Lexer::errorToken(const std::string &msg) { return makeToken(TokenType::ERROR, msg); }

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
