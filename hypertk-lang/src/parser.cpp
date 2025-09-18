#include <memory>
#include <vector>

#include "parser.hpp"
#include "lexer.hpp"
#include "error.hpp"

namespace parser
{
    using token::TokenType;

    explicit Parser::Parser(lexer::Lexer &&lexer_)
        : lexer_{std::move(lexer_)}, panicMode_{false} {}
    // Parser::Parser(const lexer::Lexer &lexer)
    //     : lexer_{std::move(lexer)}, panicMode_{false} {}

    ast::stmt::StmtPtr Parser::parse()
    {
        advance();

        while (!match(TokenType::END_OF_FILE))
        {
        }
    }

    std::optional<ast::stmt::StmtPtr> Parser::statement()
    {
        if (match(TokenType::FUNC))
            return functionStmt();

        return expressionStmt();

        // TODO: synchronization for panic handling
    }

    std::optional<ast::stmt::FunctionPtr> Parser::functionStmt()
    {
        consume(TokenType::IDENTIFIER, "Expect function name.");
        token::Token name = std::move(previous_);

        consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");

        // Read the list of argument names.
        std::vector<std::string> args;
        while (match(TokenType::IDENTIFIER))
            args.push_back(std::move(previous_.lexeme));

        consume(TokenType::RIGHT_PAREN, "Expect ')'.");
        consume(TokenType::LEFT_BRACE, "Expect '{'.");

        auto expr = expressionStmt();

        consume(TokenType::RIGHT_BRACE, "Expect '}'.");

        return std::make_unique<ast::stmt::Function>(name, std::move(args), expr.has_value() ? std::move(expr.value()) : nullptr);
    }

    std::optional<ast::stmt::ExpressionPtr> Parser::expressionStmt()
    {
        if (auto expr = expression(); expr.has_value())
        {
            auto stmt = std::make_unique<ast::stmt::Expression>(expr.value());
            consume(TokenType::SEMICOLON, "Expect ';' after expression.");
            return stmt;
        }

        return std::make_optional<ast::stmt::ExpressionPtr>();
    }

    std::optional<ast::expr::ExprPtr> Parser::expression() {}

    std::optional<ast::expr::CallPtr> Parser::callExpr() {}

    std::optional<ast::expr::BinaryPtr> Parser::binaryExpr() {}

    std::optional<ast::expr::ExprPtr> Parser::primaryExpr()
    {
        if (match(TokenType::NUMBER))
            return std::make_unique<ast::expr::Number>(std::stod(previous_.lexeme));
        if (match(TokenType::IDENTIFIER))
            return std::make_unique<ast::expr::Variable>(previous_.lexeme);

        errorAtCurrent("Unexpected token.");
        return std::make_optional<ast::expr::ExprPtr>();
    }

    void Parser::advance()
    {
        previous_ = std::move(current_);

        while (true)
        {
            current_ = lexer_.nextToken();
            if (current_.type != TokenType::ERROR)
                break;

            errorAtCurrent(current_.lexeme);
        }
    }
    void Parser::consume(token::TokenType type, const std::string &msg)
    {
        if (current_.type == type)
        {
            advance();
            return;
        }

        errorAtCurrent(msg);
    }
    /** advance if match token type */
    bool Parser::match(token::TokenType type) noexcept
    {
        if (!check(type))
        {
            return false;
        }

        advance();
        return true;
    }
    bool Parser::check(token::TokenType type) const noexcept
    {
        return current_.type == type;
    }

    void Parser::errorAtCurrent(const std::string &msg)
    {
        errorAt(current_, msg);
    }
    void Parser::error(const std::string &msg)
    {
        errorAt(previous_, msg);
    }
    void Parser::errorAt(const token::Token &t, const std::string &msg)
    {
        if (panicMode_)
            return;

        panicMode_ = true; // trigger panic mode
        error::error(t, msg);
    }
} // namespace parser
