#include <memory>
#include <vector>

#include "parser.hpp"
#include "lexer.hpp"
#include "error.hpp"

namespace parser
{
    using token::TokenType;

    Parser::Parser(lexer::Lexer &&lexer_)
        : lexer_{std::move(lexer_)}, panicMode_{false}
    {
        binopPrec_[TokenType::EQUAL] = 2;
        binopPrec_[TokenType::QUESTION_MARK] = 5;
        binopPrec_[TokenType::LESS] = 10;
        binopPrec_[TokenType::PLUS] = 20;
        binopPrec_[TokenType::MINUS] = 20;
        binopPrec_[TokenType::STAR] = 40;
        binopPrec_[TokenType::SLASH] = 40;
    }
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
        if (match(TokenType::VAR))
            return parseVariableDeclaration();
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
        if (match(TokenType::LEFT_BRACE))
            return parseBlockStmt();
        return parseExpressionStmt();
    }

    std::optional<ast::statement::VarDeclPtr> Parser::parseVariableDeclaration()
    {
        if (!match(TokenType::IDENTIFIER))
        {
            errorAtCurrent("Expect function name.");
            return std::nullopt;
        }

        std::string varName = std::move(previous_.lexeme);

        std::optional<ast::expression::ExprPtr> initializer = std::nullopt;
        if (match(TokenType::EQUAL))
        {
            if (initializer = parseExpr(); !initializer.has_value())
                return std::nullopt;
        }

        return std::make_unique<ast::statement::VarDecl>(std::move(varName),
                                                         initializer.has_value()
                                                             ? std::move(initializer.value())
                                                             : (std::optional<ast::expression::ExprPtr>)std::nullopt);
    }

    /// functionDecl
    ///   ::= id '(' id* ')'
    ///   ::= binary LETTER number? (id, id)
    std::optional<ast::statement::FunctionPtr> Parser::parseFunctionDeclaration()
    {
        std::string funcName;
        ast::FuncKind funcKind = ast::FuncKind::FUNCTION; // 0 = identifier, 1 = unary, 2 = binary.
        unsigned binPrec = 30;
        TokenType binOpType;

        switch (current_.type)
        {
        case TokenType::BINARY:
        {
            funcKind = ast::FuncKind::BINARY_OP;

            funcName = std::move(current_.lexeme);
            advance();
            binOpType = current_.type;
            funcName += current_.lexeme; // operator
            advance();

            if (match(TokenType::NUMBER))
            {
                binPrec = (unsigned)std::stoi(previous_.lexeme);
                if (binPrec < 0 || binPrec > 100)
                    errorAtCurrent("Invalid precedence: must be 1..100");
            }

            break;
        }
        case TokenType::UNARY:
        {
            funcKind = ast::FuncKind::UNARY_OP;

            funcName = std::move(current_.lexeme);
            advance();
            binOpType = current_.type;
            funcName += current_.lexeme; // operator
            advance();

            break;
        }
        case TokenType::IDENTIFIER:
        {
            funcKind = ast::FuncKind::FUNCTION;
            funcName = std::move(current_.lexeme);
            advance();
            break;
        }
        default:
        {
            errorAtCurrent("Expect function name.");
            return std::nullopt;
        }
        }

        // consume(TokenType::IDENTIFIER, "Expect function name.");
        // token::Token name = std::move(previous_);

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
        if (!check(TokenType::RIGHT_BRACE))
            stmts = std::move(parseBlock());

        consume(TokenType::RIGHT_BRACE, "Expect '}'.");

        switch (funcKind)
        {
        case ast::FuncKind::BINARY_OP:
        {
            setTokenPrecedence(binOpType, binPrec);
            return std::make_unique<ast::statement::BinOpDef>(std::move(funcName), std::move(args), std::move(stmts), binPrec);
        }
        case ast::FuncKind::UNARY_OP:
            return std::make_unique<ast::statement::UnaryOpDef>(std::move(funcName), std::move(args), std::move(stmts));
        default:
            return std::make_unique<ast::statement::Function>(std::move(funcName), std::move(args), std::move(stmts));
        }
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

    std::optional<ast::statement::BlockPtr> Parser::parseBlockStmt()
    {
        std::vector<ast::statement::StmtPtr> statements = std::move(parseBlock());
        consume(TokenType::RIGHT_BRACE, "Expect '}' at the end of block.");
        return std::make_unique<ast::statement::Block>(std::move(statements));
    }

    inline std::vector<ast::statement::StmtPtr> Parser::parseBlock()
    {
        std::vector<ast::statement::StmtPtr> statements;

        while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE))
            if (auto stmt = parseDeclaration(); stmt.has_value())
                statements.push_back(std::move(stmt.value()));

        return std::move(statements);
    }
    //>

    //> Parse expression
    std::optional<ast::expression::ExprPtr> Parser::parseExpr() { return parseExpr(0); }
    std::optional<ast::expression::ExprPtr> Parser::parseExpr(int exprPrec)
    {
        auto LHS = parseUnary();
        if (!LHS.has_value())
            return std::nullopt;

        return parseBinaryRHS(0, std::move(LHS.value()));
    }

    /// unary
    ///   ::= primary
    ///   ::= '!' unary
    std::optional<ast::expression::ExprPtr> Parser::parseUnary()
    {
        // If the current token is not an unary operator, it must be a primary expr.
        if (!ast::isUnaryOp(current_.type))
            return parsePrimary();

        ast::UnaryOp op = static_cast<ast::UnaryOp>(current_.type);
        advance();

        if (auto operand = parseUnary(); operand.has_value())
            return std::make_unique<ast::expression::Unary>(op, std::move(operand.value()));

        return std::nullopt;
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
            auto RHS = parseUnary();
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
        int prec = binopPrec_[type];
        if (prec <= 0)
            return -1;

        return prec;
    }
    bool Parser::setTokenPrecedence(TokenType type, int prec)
    {
        binopPrec_[type] = prec;
        return true;
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
