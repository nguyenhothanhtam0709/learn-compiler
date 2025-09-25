#include "semantic_analyzer.hpp"

namespace semantic_analysis
{
    BasicSemanticAnalyzer::BasicSemanticAnalyzer(const ast::Program &program)
        : program_{program} {}

    bool BasicSemanticAnalyzer::analyze()
    {
        for (const auto &stmt : program_)
        {
            if (!visit(stmt))
                return false;
        }

        return true;
    }

    //> Print statements
    bool BasicSemanticAnalyzer::visitVarDeclStmt(
        const ast::statement::VarDecl &stmt) { return true; }

    bool BasicSemanticAnalyzer::visitFunctionStmt(
        const ast::statement::Function &stmt) { return true; }

    bool BasicSemanticAnalyzer::visitBinOpDefStmt(
        const ast::statement::BinOpDef &stmt) { return true; }

    bool BasicSemanticAnalyzer::visitUnaryOpDefStmt(
        const ast::statement::UnaryOpDef &stmt) { return true; }

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
        const ast::statement::For &stmt) { return true; }
    //<

    //> Print expressions
    bool BasicSemanticAnalyzer::visitNumberExpr(
        const ast::expression::Number &expr) { return true; }

    bool BasicSemanticAnalyzer::visitVariableExpr(
        const ast::expression::Variable &expr) { return true; }

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
        // if (!visit(expr.Callee))
        //     return false;

        for (const auto &expr_ : expr.Args)
            if (!visit(expr_))
                return false;

        return true;
    }
    //<
} // namespace hypertk
