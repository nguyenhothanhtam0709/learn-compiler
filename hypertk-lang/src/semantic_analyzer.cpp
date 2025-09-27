#include "semantic_analyzer.hpp"
#include "error.hpp"

namespace semantic_analysis
{
    BasicSemanticAnalyzer::BasicSemanticAnalyzer(const ast::Program &program)
        : program_{program} {}

    bool BasicSemanticAnalyzer::analyze()
    {
        beginScope();
        for (const auto &stmt : program_)
            if (!visit(stmt))
                return false;
        endScope();

        return true;
    }

    //> Print statements
    bool BasicSemanticAnalyzer::visitBlockStmt(
        const ast::statement::Block &stmt)
    {
        beginScope();
        for (const auto &stmt_ : stmt.Statements)
            if (!visit(stmt_))
                return false;
        endScope();

        return true;
    }

    bool BasicSemanticAnalyzer::visitVarDeclStmt(
        const ast::statement::VarDecl &stmt)
    {
        if (!declare(stmt.VarName))
            return false;
        if (stmt.Initializer.has_value() && !visit(stmt.Initializer.value()))
            return false;
        if (!define(stmt.VarName))
            return false;

        return true;
    }

    bool BasicSemanticAnalyzer::visitFunctionStmt(
        const ast::statement::Function &stmt)
    {
        if (!declare(stmt.Name) || !define(stmt.Name))
            return false;

        return resolveFunctionBody(stmt);
    }

    bool BasicSemanticAnalyzer::visitBinOpDefStmt(
        const ast::statement::BinOpDef &stmt)
    {
        if (!declare(stmt.Name) || !define(stmt.Name))
            return false;

        if (stmt.Params.size() != 2)
        {
            error::error(stmt.Name, "Binary operator must have 2 operands");
            return false;
        }

        return resolveFunctionBody(stmt);
    }

    bool BasicSemanticAnalyzer::visitUnaryOpDefStmt(
        const ast::statement::UnaryOpDef &stmt)
    {
        if (!declare(stmt.Name) || !define(stmt.Name))
            return false;

        if (stmt.Params.size() != 1)
        {
            error::error(stmt.Name, "Unary operator must have 1 operand");
            return false;
        }

        return resolveFunctionBody(stmt);
    }

    bool BasicSemanticAnalyzer::visitExpressionStmt(
        const ast::statement::Expression &stmt)
    {
        return visit(stmt.Expr);
    }

    bool BasicSemanticAnalyzer::visitReturnStmt(
        const ast::statement::Return &stmt)
    {
        return visit(stmt.Expr);
    }

    bool BasicSemanticAnalyzer::visitIfStmt(
        const ast::statement::If &stmt)
    {
        if (!visit(stmt.Cond))
            return false;
        if (!visit(stmt.Then))
            return false;
        if (stmt.Else.has_value() && !visit(stmt.Else.value()))
            return false;
        return true;
    }

    bool BasicSemanticAnalyzer::visitForStmt(
        const ast::statement::For &stmt)
    {
        beginScope();
        if (!declare(stmt.VarName) ||
            !define(stmt.VarName) ||
            !visit(stmt.Start) ||
            !visit(stmt.End) ||
            !visit(stmt.Step) ||
            !visit(stmt.Body))
            return false;
        endScope();
        return true;
    }
    //<

    //> Print expressions
    bool BasicSemanticAnalyzer::visitNumberExpr(
        const ast::expression::Number &expr) { return true; }

    bool BasicSemanticAnalyzer::visitVariableExpr(
        const ast::expression::Variable &expr)
    {
        if (!scopes_.empty())
            for (auto scope = scopes_.crbegin(); scope != scopes_.crend(); ++scope)
                if (scope->find(expr.Name.lexeme) != scope->end())
                    return true;

        error::error(expr.Name, "Unknown variable");
        return false;
    }

    bool BasicSemanticAnalyzer::visitBinaryExpr(
        const ast::expression::Binary &expr)
    {
        return visit(expr.LHS) && visit(expr.RHS);
    }

    bool BasicSemanticAnalyzer::visitUnaryExpr(
        const ast::expression::Unary &expr)
    {
        return visit(expr.Operand);
    }

    bool BasicSemanticAnalyzer::visitConditionalExpr(
        const ast::expression::Conditional &expr)
    {
        return visit(expr.Cond) && visit(expr.Then) && visit(expr.Else);
    }

    bool BasicSemanticAnalyzer::visitCallExpr(
        const ast::expression::Call &expr)
    {
        // if (!visitVariableExpr(*expr.Callee))
        //     return false;

        for (const auto &expr_ : expr.Args)
            if (!visit(expr_))
                return false;

        return true;
    }
    //<

    inline bool BasicSemanticAnalyzer::resolveFunctionBody(
        const ast::statement::Function &stmt)
    {
        beginScope();
        for (const auto &param : stmt.Params)
            if (!declare(param) || !define(param))
                return false;
        for (const auto &stmt_ : stmt.Body)
            if (!visit(stmt_))
                return false;
        endScope();

        return true;
    }
    inline void BasicSemanticAnalyzer::beginScope() { scopes_.emplace_back(); }
    inline void BasicSemanticAnalyzer::endScope() { scopes_.pop_back(); }
    inline bool BasicSemanticAnalyzer::declare(const token::Token &name)
    {
        if (scopes_.empty())
            return false;

        auto &scope = scopes_.back();
        if (scope.find(name.lexeme) != scope.end())
        {
            error::error(name.line, "Already a variable with this name in this scope.");
            return false;
        }

        scope[name.lexeme] = false;
        return true;
    }
    inline bool BasicSemanticAnalyzer::define(const token::Token &name)
    {
        if (scopes_.empty())
            return false;

        scopes_.back()[name.lexeme] = true;
        return true;
    }
} // namespace hypertk
