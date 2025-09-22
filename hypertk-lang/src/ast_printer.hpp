#ifndef HYPERTK_AST_PRINTER_HPP
#define HYPERTK_AST_PRINTER_HPP

#include <string>

#include "ast.hpp"

namespace ast
{
    class SimplePrinter : protected statement::Visitor<void>, protected expression::Visitor<void>
    {
    private:
        int indent_;

    public:
        explicit SimplePrinter();

        void print(const Program &program);

    protected:
        using expression::Visitor<void>::visit;
        using statement::Visitor<void>::visit;

        //> Print statements
        void visitFunctionStmt(const statement::Function &stmt);
        void visitBinOpDefStmt(const statement::BinOpDef &stmt);
        void visitUnaryOpDefStmt(const statement::UnaryOpDef &stmt);
        void visitExpressionStmt(const statement::Expression &stmt);
        void visitReturnStmt(const statement::Return &stmt);
        void visitIfStmt(const statement::If &stmt);
        void visitForStmt(const statement::For &stmt);
        //<

        //> Print expressions
        void visitNumberExpr(const expression::Number &expr);
        void visitVariableExpr(const expression::Variable &expr);
        void visitBinaryExpr(const expression::Binary &expr);
        void visitUnaryExpr(const expression::Unary &expr);
        void visitConditionalExpr(const expression::Conditional &expr);
        void visitCallExpr(const expression::Call &expr);
        //<

        std::string op(ast::BinaryOp op) const noexcept;
        void increaseIndent() noexcept;
        void decreaseIndent() noexcept;
        void printIndent() const noexcept;
    };
} // namespace ast

#endif