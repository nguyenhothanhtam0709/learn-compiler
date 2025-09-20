#include <iostream>

#include "ast_printer.hpp"
#include "ast.hpp"

namespace ast
{
    SimplePrinter::SimplePrinter() : statement::Visitor<void>(), expression::Visitor<void>(), indent_{0} {}

    void SimplePrinter::print(const Program &program)
    {
        for (const auto &stmt : program)
        {
            visit(stmt);
        }
    }

    //> Print statements
    void SimplePrinter::visitFunctionStmt(const statement::Function &stmt)
    {
        printIndent();
        std::cout << "FunctionDeclaration [ " << stmt.Name << " ]  Arguments: ";
        for (const auto &arg : stmt.Args)
            std::cout << arg << " ";
        std::cout << '\n';

        increaseIndent();
        for (const auto &stmt : stmt.Body)
            visit(stmt);
        decreaseIndent();
    }

    void SimplePrinter::visitExpressionStmt(const statement::Expression &stmt)
    {
        printIndent();
        std::cout << "ExpressionStatement\n";

        increaseIndent();
        visit(stmt.Expr);
        decreaseIndent();
    }

    void SimplePrinter::visitReturnStmt(const statement::Return &stmt)
    {
        printIndent();
        std::cout << "ReturnStatement\n";

        increaseIndent();
        visit(stmt.Expr);
        decreaseIndent();
    }

    void SimplePrinter::visitIfStmt(const statement::If &stmt)
    {
        printIndent();
        std::cout << "IfStatement\n";

        printIndent();
        std::cout << "Condition: \n";
        increaseIndent();
        visit(stmt.Cond);
        decreaseIndent();

        printIndent();
        std::cout << "Then: \n";
        increaseIndent();
        visit(stmt.Then);
        decreaseIndent();

        if (stmt.Else.has_value())
        {
            printIndent();
            std::cout << "Else: \n";
            increaseIndent();
            visit(stmt.Else.value());
            decreaseIndent();
        }
    }
    //<

    //> Print expressions
    void SimplePrinter::visitNumberExpr(const expression::Number &expr)
    {
        printIndent();
        std::cout << "Number [ " << expr.Val << " ]\n";
    }

    void SimplePrinter::visitVariableExpr(const expression::Variable &expr)
    {
        printIndent();
        std::cout << "Variable [ " << expr.Name << " ]\n";
    }

    void SimplePrinter::visitBinaryExpr(const expression::Binary &expr)
    {
        printIndent();
        std::cout << "Binary [ " << op(expr.Op) << " ]\n";

        increaseIndent();
        visit(expr.LHS);
        visit(expr.RHS);
        decreaseIndent();
    }

    void SimplePrinter::visitCallExpr(const expression::Call &expr) {}
    //<

    std::string SimplePrinter::op(ast::BinaryOp op) const noexcept
    {
        switch (op)
        {
        case ast::BinaryOp::ADD:
            return "+";
        case ast::BinaryOp::SUB:
            return "-";
        case ast::BinaryOp::MUL:
            return "*";
        case ast::BinaryOp::DIV:
            return "/";
        default:
            return "unknown";
        }
    }
    void SimplePrinter::increaseIndent() noexcept { indent_++; }
    void SimplePrinter::decreaseIndent() noexcept { indent_--; }
    void SimplePrinter::printIndent() const noexcept
    {
        for (int i = 0; i < indent_; i++)
        {
            std::cout << "    ";
        }
    }
} // namespace ast
