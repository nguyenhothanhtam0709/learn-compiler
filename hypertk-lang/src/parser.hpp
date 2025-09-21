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

        //> Parse statement
        std::optional<ast::statement::StmtPtr> parseDeclaration();
        std::optional<ast::statement::StmtPtr> parseStatement();
        std::optional<ast::statement::FunctionPtr> parseFunctionDeclaration();
        std::optional<ast::statement::ExpressionPtr> parseExpressionStmt();
        std::optional<ast::statement::ReturnPtr> parseReturnStmt();
        std::optional<ast::statement::IfPtr> parseIfStmt();
        std::optional<ast::statement::ForPtr> parseForStmt();
        //<

        //> Parse expression
        std::optional<ast::expression::ExprPtr> parseExpr();
        std::optional<ast::expression::ExprPtr> parseExpr(int exprPrec);
        std::optional<ast::expression::ExprPtr> parseBinaryRHS(int exprPrec, ast::expression::ExprPtr LHS);
        std::optional<ast::expression::ExprPtr> parsePrimary();
        std::optional<ast::expression::ExprPtr> parseParen();
        std::optional<ast::expression::ExprPtr> parseIdentifier();
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