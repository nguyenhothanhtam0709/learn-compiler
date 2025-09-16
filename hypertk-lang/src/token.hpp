#ifndef HYPERTK_TOKEN_HPP
#define HYPERTK_TOKEN_HPP

#include <string>

namespace hypertk
{
    enum class TokenType
    {
        /** character tokens */
        LEFT_PAREN,  // `(`
        RIGHT_PAREN, // ')'
        LEFT_BRACE,  // `{`
        RIGHT_BRACE, // `}`
        PLUS,        // `+`
        MINUS,       // `-`
        STAR,        // `*`
        SLASH,       // `/`
        COLON,       // `:`
        SEMICOLON,   // `;`
        /** literals */
        IDENTIFIER, // Identifier
        NUMBER,     // float
        /** keywords */
        FUNC, // `func`
        /** other */
        ERROR, // Present error
        END_OF_FILE,
    };

    class Token
    {
    public:
        TokenType type;
        std::string lexeme;
        int line;

        Token(TokenType type, int line);
        Token(TokenType type, const std::string &lexeme, int line);

        Token(Token &&other) noexcept;
        Token &operator=(Token &&other) noexcept;

        // Let compiler generate copy operations
        Token(const Token &) = default;
        Token &operator=(const Token &) = default;
    };
}

#endif