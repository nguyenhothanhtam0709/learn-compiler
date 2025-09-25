#ifndef HYPERTK_TOKEN_HPP
#define HYPERTK_TOKEN_HPP

#include <string>

#include "common.hpp"

namespace token
{
    enum class TokenType
    {
        /** character tokens */
        LEFT_PAREN,    // `(`
        RIGHT_PAREN,   // ')'
        LEFT_BRACE,    // `{`
        RIGHT_BRACE,   // `}`
        PLUS,          // `+`
        MINUS,         // `-`
        STAR,          // `*`
        SLASH,         // `/`
        EQUAL,         // `=`
        LESS,          // `<`
        GREATER,       // `>`
        EXCLAMATION,   // `!`
        QUESTION_MARK, // `?`
        COLON,         // `:`
        SEMICOLON,     // `;`
        COMMA,         // `,`
        VERTICAL_BAR,  // `|`
        AMPERSAND,     // `&`
        /** literals */
        IDENTIFIER, // Identifier
        NUMBER,     // float
        /** keywords */
        FUNC,   // `func`
        RETURN, // `return`
        IF,     // `if`
        THEN,   // `then`
        ELSE,   // `else`
        FOR,    // `for`
        IN,     // `in`
        UNARY,  // `unary`
        BINARY, // `binary`
        VAR,    // `var`
        /** other */
        ERROR, // Present error
        END_OF_FILE,
    };

    class Token : private Uncopyable
    {
    public:
        TokenType type;
        std::string lexeme;
        int line;

        explicit Token();
        Token(TokenType type, int line);
        Token(TokenType type, const std::string &lexeme, int line);

        Token(Token &&other) noexcept;
        Token &operator=(Token &&other) noexcept;
    };
}

#endif