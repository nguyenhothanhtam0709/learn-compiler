#ifndef HYPERTK_LEXER_HPP
#define HYPERTK_LEXER_HPP

#include <string>

#include "token.hpp"

namespace lexer
{
    class Lexer : private Uncopyable
    {
    public:
        explicit Lexer(const std::string &src);

        Lexer(Lexer &&other);
        Lexer &operator=(Lexer &&other);

        /**
         * @note Since C++17, compilers guarantee `Return Value Optimization (RVO)` in most cases.
         */
        token::Token nextToken();

    private:
        /** @brief Text source code */
        std::string src_;
        int start_;
        int current_;
        int line_;

        token::Token identifier();
        token::Token number();

        void skipWhitespaceAndComment() noexcept;

        inline token::Token makeToken(token::TokenType type);
        inline token::Token makeToken(token::TokenType type, const std::string &lexeme);
        inline std::string makeLexeme();
        inline token::Token errorToken(const std::string &msg);

        bool match(char expected) noexcept;
        inline char advance() noexcept;
        inline char peek() const noexcept;
        inline char peekNext() const noexcept;
        inline bool isAtEnd() const noexcept;
        inline bool nextIsEnd() const noexcept;
    };
}

#endif