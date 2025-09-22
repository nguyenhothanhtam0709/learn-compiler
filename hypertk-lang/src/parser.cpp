#include <memory>
#include <vector>

#include "parser.hpp"
#include "lexer.hpp"
#include "error.hpp"

namespace parser
{
    using token::TokenType;

    Parser::Parser(lexer::Lexer &&lexer_)
        : lexer_{std::move(lexer_)}, panicMode_{false} {}
    // Parser::Parser(const lexer::Lexer &lexer)
    //     : lexer_{std::move(lexer)}, panicMode_{false} {}

    std::optional<ast::Program> Parser::parse()
    {
        advance();

        ast::Program program;
        while (!match(TokenType::END_OF_FILE))
        {
            if (auto stmt = parseDeclaration(); stmt.has_value())
            {
                program.emplace_back(std::move(stmt.value()));
                continue;
            }

            break; // should synchronize
        }

        return std::move(program);
    }

    //> Parse statement
    std::optional<ast::statement::StmtPtr> Parser::parseDeclaration()
    {
        if (match(TokenType::FUNC))
            return parseFunctionDeclaration();

        return parseStatement();
    }

    std::optional<ast::statement::StmtPtr> Parser::parseStatement()
    {
        if (match(TokenType::RETURN))
            return parseReturnStmt();
        if (match(TokenType::IF))
            return parseIfStmt();
        if (match(TokenType::FOR))
            return parseForStmt();
        return parseExpressionStmt();
    }

    std::optional<ast::statement::FunctionPtr> Parser::parseFunctionDeclaration()
    {
        consume(TokenType::IDENTIFIER, "Expect function name.");
        token::Token name = std::move(previous_);

        consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");

        // Read the list of argument names.
        std::vector<std::string> args;
        if (!check(TokenType::RIGHT_PAREN))
        {
            do
            {
                advance();
                args.push_back(std::move(previous_.lexeme));
            } while (match(TokenType::COMMA));
        }

        consume(TokenType::RIGHT_PAREN, "Expect ')'.");
        consume(TokenType::LEFT_BRACE, "Expect '{'.");

        std::vector<ast::statement::StmtPtr> stmts;
        while (!check(TokenType::RIGHT_BRACE))
        {
            if (auto stmt = parseStatement(); stmt.has_value())
            {
                stmts.emplace_back(std::move(stmt.value()));
                continue;
            }
        }

        consume(TokenType::RIGHT_BRACE, "Expect '}'.");

        return std::make_unique<ast::statement::Function>(std::move(name.lexeme), std::move(args), std::move(stmts));
    }

    std::optional<ast::statement::ExpressionPtr> Parser::parseExpressionStmt()
    {
        if (auto expr = parseExpr(); expr.has_value())
        {
            auto stmt = std::make_unique<ast::statement::Expression>(std::move(expr.value()));
            consume(TokenType::SEMICOLON, "Expect ';' after expression.");
            return stmt;
        }

        return std::nullopt;
    }

    std::optional<ast::statement::ReturnPtr> Parser::parseReturnStmt()
    {
        if (auto expr = parseExpr(); expr.has_value())
        {
            auto stmt = std::make_unique<ast::statement::Return>(std::move(expr.value()));
            consume(TokenType::SEMICOLON, "Expect ';' after expression.");
            return stmt;
        }

        return std::nullopt;
    }

    std::optional<ast::statement::IfPtr> Parser::parseIfStmt()
    {
        consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
        auto cond = parseExpr();
        if (!cond.has_value())
            return std::nullopt;
        consume(TokenType::RIGHT_PAREN, "Expect ')' after if condition.");

        auto then_ = parseStatement();
        if (!then_.has_value())
            return std::nullopt;

        std::optional<ast::statement::StmtPtr> else_ = std::nullopt;
        if (match(TokenType::ELSE))
        {
            if (else_ = parseStatement(); !else_.has_value())
                return std::nullopt;
        }

        return std::make_unique<ast::statement::If>(
            std::move(cond.value()),
            std::move(then_.value()),
            else_.has_value()
                ? std::move(else_.value())
                : (std::optional<ast::statement::StmtPtr>)std::nullopt);
    }

    /// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
    std::optional<ast::statement::ForPtr> Parser::parseForStmt()
    {
        consume(TokenType::IDENTIFIER, "expected identifier after for");
        token::Token nameToken = std::move(previous_);

        consume(TokenType::EQUAL, "expect '=' after variable name.");

        auto start = parseExpr();
        if (!start.has_value())
            return std::nullopt;

        consume(TokenType::COMMA, "expected ',' after for start value");

        auto end = parseExpr();
        if (!end.has_value())
            return std::nullopt;

        consume(TokenType::COMMA, "expected ',' after for end value");

        auto step = parseExpr();
        if (!step.has_value())
            return std::nullopt;

        consume(TokenType::IN, "expected 'in' after for step value");

        auto body = parseStatement();
        if (!body.has_value())
            return std::nullopt;

        return std::make_unique<ast::statement::For>(std::move(nameToken.lexeme),
                                                     std::move(start.value()),
                                                     std::move(end.value()),
                                                     std::move(step.value()),
                                                     std::move(body.value()));
    }
    //>

    //> Parse expression
    std::optional<ast::expression::ExprPtr> Parser::parseExpr() { return parseExpr(0); }
    std::optional<ast::expression::ExprPtr> Parser::parseExpr(int exprPrec)
    {
        auto LHS = parsePrimary();
        if (!LHS.has_value())
            return std::nullopt;

        return parseBinaryRHS(0, std::move(LHS.value()));
    }

    std::optional<ast::expression::ExprPtr> Parser::parseBinaryRHS(int exprPrec, ast::expression::ExprPtr LHS)
    {
        // If this is a binop, find its precedence.
        while (true)
        {
            int tokenPrec = getTokenPrecedence(current_.type);

            // If this is a binop that binds at least as tightly as the current binop,
            // consume it, otherwise we are done.
            if (tokenPrec < exprPrec)
                return LHS;

            //> Parse conditional expression
            if (match(TokenType::QUESTION_MARK))
            {
                auto thenExpr = parseExpr(tokenPrec);
                if (!thenExpr.has_value())
                    return std::nullopt;

                consume(TokenType::COLON, "Expect ':' in conditional expression");

                auto elseExpr = parseExpr(tokenPrec);
                if (!elseExpr.has_value())
                    return std::nullopt;

                LHS = std::make_unique<ast::expression::Conditional>(std::move(LHS),
                                                                     std::move(thenExpr.value()),
                                                                     std::move(elseExpr.value()));
                continue;
            }
            //<

            // Okay, we know this is a binop.
            advance();
            auto binOp = std::move(previous_);

            // Parse the primary expression after the binary operator.
            auto RHS = parsePrimary();
            if (!RHS.has_value())
                return std::nullopt;

            // If BinOp binds less tightly with RHS than the operator after RHS, let
            // the pending operator take RHS as its LHS.
            int nextPrec = getTokenPrecedence(current_.type);
            if (tokenPrec < nextPrec)
            {
                RHS = parseBinaryRHS(tokenPrec + 1, std::move(RHS.value()));
                if (!RHS.has_value())
                    return std::nullopt;
            }

            // Merge LHS/RHS.
            LHS = std::make_unique<ast::expression::Binary>((ast::BinaryOp)binOp.type, std::move(LHS), std::move(RHS.value()));
        }
    }

    std::optional<ast::expression::ExprPtr> Parser::parsePrimary()
    {
        if (match(TokenType::NUMBER))
            return std::make_unique<ast::expression::Number>(std::stod(previous_.lexeme));
        if (match(TokenType::IDENTIFIER))
            return parseIdentifier();
        if (match(TokenType::LEFT_PAREN))
            return parseParen();

        errorAtCurrent("Unexpected token.");
        return std::nullopt;
    }

    std::optional<ast::expression::ExprPtr> Parser::parseIdentifier()
    {
        token::Token name = std::move(previous_);
        if (!match(TokenType::LEFT_PAREN))
            return std::make_unique<ast::expression::Variable>(std::move(name.lexeme));

        //> Parse call expression
        std::vector<ast::expression::ExprPtr> args;
        if (!check(TokenType::RIGHT_PAREN))
        {
            do
            {
                if (auto expr_ = parseExpr(); expr_.has_value())
                {
                    args.push_back(std::move(expr_.value()));
                    continue;
                }

                return std::nullopt; // Have error
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')'");

        return std::make_unique<ast::expression::Call>(std::move(name.lexeme), std::move(args));
        //<
    }

    /// @brief parenexpr ::= '(' expression ')'
    std::optional<ast::expression::ExprPtr> Parser::parseParen()
    {
        auto expr = parseExpr();
        if (!expr.has_value())
            return expr;

        consume(TokenType::RIGHT_PAREN, "Expect ')'.");
        return expr;
    }

    int Parser::getTokenPrecedence(TokenType type)
    {
        switch (type)
        {
        case TokenType::QUESTION_MARK:
            return 5;
        case TokenType::LESS:
            return 10;
        case TokenType::PLUS:
            return 20;
        case TokenType::MINUS:
            return 20;
        case TokenType::STAR:
            return 40;
        case TokenType::SLASH:
            return 40;
        default:
            return -1;
        }
    }
    //< Parse expression

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
        if (check(type))
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
