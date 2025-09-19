#include <string>
#include <iostream>

#include "error.hpp"
#include "token.hpp"

namespace error
{
    static bool hasError_ = false;

    bool hasError() { return hasError_; }

    static inline void report(const int line, const std::string where, const std::string msg)
    {
        hasError_ = true;
        std::cerr << "[line " << line << "] Error " << where << ": " << msg << "\n";
    }

    void error(const int line, const std::string &msg)
    {
        report(line, "", msg);
    }

    void error(const token::Token &t, const std::string &msg)
    {
        if (t.type == token::TokenType::END_OF_FILE)
        {
            report(t.line, "at end", msg);
        }
        else if (t.type == token::TokenType::ERROR)
        {
            report(t.line, "", t.lexeme);
        }
        else
        {
            report(t.line, std::string("at '") + t.lexeme + "'", msg);
        }
    }
} // namespace error
