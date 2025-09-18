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

        std::optional<ast::Program> parse();

    private:
        lexer::Lexer lexer_;
        token::Token previous_;
        token::Token current_;
        bool panicMode_;

        std::optional<ast::stmt::StmtPtr> parseDeclaration();
        std::optional<ast::stmt::StmtPtr> parseStatement();
        std::optional<ast::stmt::FunctionPtr> parseFunctionDeclaration();
        std::optional<ast::stmt::ExpressionPtr> parseExpressionStmt();

        //> Parse expression
        std::optional<ast::expr::ExprPtr> parseExpr();
        std::optional<ast::expr::ExprPtr> parseBinaryRHS(int exprPrec, ast::expr::ExprPtr LHS);
        std::optional<ast::expr::ExprPtr> parsePrimary();
        std::optional<ast::expr::ExprPtr> parseParen();
        int getTokenPrecedence(token::TokenType type);
        //< Parse expression

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