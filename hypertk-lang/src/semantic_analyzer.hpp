#ifndef HYPERTK_SEMANTIC_ANALYZER_HPP
#define HYPERTK_SEMANTIC_ANALYZER_HPP

#include "common.hpp"
#include "ast.hpp"

namespace semantic_analysis
{
    class BasicSemanticAnalyzer
        : private Uncopyable,
          protected ast::statement::Visitor<bool>,
          protected ast::expression::Visitor<bool>
    {
    public:
        explicit BasicSemanticAnalyzer(const ast::Program &program);

        /** @brief Return `false` if having errors */
        bool analyze();

    private:
        const ast::Program &program_;

    protected:
        using ast::statement::Visitor<bool>::visit;
        using ast::expression::Visitor<bool>::visit;

        //> Print statements
        bool visitBlockStmt(const ast::statement::Block &stmt);
        bool visitVarDeclStmt(const ast::statement::VarDecl &stmt);
        bool visitFunctionStmt(const ast::statement::Function &stmt);
        bool visitBinOpDefStmt(const ast::statement::BinOpDef &stmt);
        bool visitUnaryOpDefStmt(const ast::statement::UnaryOpDef &stmt);
        bool visitExpressionStmt(const ast::statement::Expression &stmt);
        bool visitReturnStmt(const ast::statement::Return &stmt);
        bool visitIfStmt(const ast::statement::If &stmt);
        bool visitForStmt(const ast::statement::For &stmt);
        //<

        //> Print expressions
        bool visitNumberExpr(const ast::expression::Number &expr);
        bool visitVariableExpr(const ast::expression::Variable &expr);
        bool visitBinaryExpr(const ast::expression::Binary &expr);
        bool visitUnaryExpr(const ast::expression::Unary &expr);
        bool visitConditionalExpr(const ast::expression::Conditional &expr);
        bool visitCallExpr(const ast::expression::Call &expr);
        //<
    };
} // namespace hypertk

#endif