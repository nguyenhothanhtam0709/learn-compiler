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
        void visitExpressionStmt(const statement::Expression &stmt);
        //<

        //> Print expressions
        void visitNumberExpr(const expression::Number &expr);
        void visitVariableExpr(const expression::Variable &expr);
        void visitBinaryExpr(const expression::Binary &expr);
        void visitCallExpr(const expression::Call &expr);
        //<

        std::string op(ast::BinaryOp op) const noexcept;
        void increaseIndent() noexcept;
        void decreaseIndent() noexcept;
        void printIndent()const noexcept;
    };
} // namespace ast

#endif