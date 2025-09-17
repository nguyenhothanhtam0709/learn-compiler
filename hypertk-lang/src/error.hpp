#ifndef HYPERTK_ERROR_HPP
#define HYPERTK_ERROR_HPP

#include <string>

#include "token.hpp"

namespace error
{
    bool hasError();

    void error(const int line, const std::string &msg);
    void error(const token::Token &t, const std::string &msg);
} // namespace error

#endif