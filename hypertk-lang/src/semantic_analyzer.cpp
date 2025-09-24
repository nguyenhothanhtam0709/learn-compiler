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
    bool BasicSemanticAnalyzer::visitFunctionStmt(
        const ast::statement::Function &stmt) { return true; }
    bool BasicSemanticAnalyzer::visitBinOpDefStmt(
        const ast::statement::BinOpDef &stmt) { return true; }
    bool BasicSemanticAnalyzer::visitUnaryOpDefStmt(
        const ast::statement::UnaryOpDef &stmt) { return true; }
    bool BasicSemanticAnalyzer::visitExpressionStmt(
        const ast::statement::Expression &stmt) { return true; }
    bool BasicSemanticAnalyzer::visitReturnStmt(
        const ast::statement::Return &stmt) { return true; }
    bool BasicSemanticAnalyzer::visitIfStmt(
        const ast::statement::If &stmt) { return true; }
    bool BasicSemanticAnalyzer::visitForStmt(
        const ast::statement::For &stmt) { return true; }
    //<

    //> Print expressions
    bool BasicSemanticAnalyzer::visitNumberExpr(
        const ast::expression::Number &expr) { return true; }
    bool BasicSemanticAnalyzer::visitVariableExpr(
        const ast::expression::Variable &expr) { return true; }
    bool BasicSemanticAnalyzer::visitBinaryExpr(
        const ast::expression::Binary &expr) { return true; }
    bool BasicSemanticAnalyzer::visitUnaryExpr(
        const ast::expression::Unary &expr) { return true; }
    bool BasicSemanticAnalyzer::visitConditionalExpr(
        const ast::expression::Conditional &expr) { return true; }
    bool BasicSemanticAnalyzer::visitCallExpr(
        const ast::expression::Call &expr) { return true; }
    //<
} // namespace hypertk
