#ifndef HYPERTK_PARSER_HPP
#define HYPERTK_PARSER_HPP

#include <optional>

#include "token.hpp"
#include "ast.hpp"
#include "lexer.hpp"

namespace parser
{
    class Parser
    {
    public:
        explicit Parser(lexer::Lexer &&lexer_);
        // explicit Parser(const lexer::Lexer &lexer_);

        ast::stmt::StmtPtr parse();

    private:
        lexer::Lexer lexer_;
        token::Token previous_;
        token::Token current_;
        bool panicMode_;

        std::optional<ast::stmt::StmtPtr> statement();
        std::optional<ast::stmt::FunctionPtr> functionStmt();
        std::optional<ast::stmt::ExpressionPtr> expressionStmt();

        std::optional<ast::expr::ExprPtr> expression();
        std::optional<ast::expr::CallPtr> callExpr();
        std::optional<ast::expr::BinaryPtr> binaryExpr();
        std::optional<ast::expr::ExprPtr> primaryExpr();

        void advance();
        void consume(token::TokenType type, const std::string &msg);
        bool match(token::TokenType type) noexcept;
        bool check(token::TokenType type) const noexcept;

        void errorAtCurrent(const std::string &msg);
        void error(const std::string &msg);
        void errorAt(const token::Token &t, const std::string &msg);
    };
} // namespace parser

#endif