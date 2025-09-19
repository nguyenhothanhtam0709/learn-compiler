#ifndef HYPERTK_AST_HPP
#define HYPERTK_AST_HPP

#include <string>
#include <vector>
#include <variant>
#include <memory>

#include "common.hpp"
#include "token.hpp"

namespace ast
{
    enum class BinaryOp
    {
        ADD = (int)token::TokenType::PLUS,
        SUB = (int)token::TokenType::MINUS,
        MUL = (int)token::TokenType::STAR,
        DIV = (int)token::TokenType::SLASH,
    };

    /** @brief Expression ast */
    namespace expression
    {
        struct Number;
        struct Variable;
        struct Binary;
        struct Call;

        using NumberPtr = std::unique_ptr<Number>;
        using VariablePtr = std::unique_ptr<Variable>;
        using BinaryPtr = std::unique_ptr<Binary>;
        using CallPtr = std::unique_ptr<Call>;
        using ExprPtr = std::variant<NumberPtr, VariablePtr, BinaryPtr, CallPtr>;

        struct Number : private Uncopyable
        {
            double Val;

            explicit Number(double val) : Val{val} {}
        };

        struct Variable : private Uncopyable
        {
            std::string Name;

            explicit Variable(const std::string &name) : Name{name} {}
        };

        struct Binary : private Uncopyable
        {
            BinaryOp Op;
            ExprPtr LHS, RHS;

            Binary(BinaryOp Op, ExprPtr LHS, ExprPtr RHS)
                : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
        };

        struct Call : private Uncopyable
        {
            std::string Callee;
            std::vector<ExprPtr> Args;

            Call(const std::string &Callee,
                 std::vector<ExprPtr> Args)
                : Callee(Callee), Args(std::move(Args)) {}
        };

        template <typename R>
        class Visitor
        {
        public:
            /** @brief Visit expression node */
            R visit(const ExprPtr &expr)
            {
                return std::visit(
                    overloaded{
                        [this](const NumberPtr &expr)
                        {
                            return visitNumberExpr(*expr);
                        },
                        [this](const VariablePtr &expr)
                        {
                            return visitVariableExpr(*expr);
                        },
                        [this](const BinaryPtr &expr)
                        {
                            return visitBinaryExpr(*expr);
                        },
                        [this](const CallPtr &expr)
                        {
                            return visitCallExpr(*expr);
                        }},
                    expr);
            }

        protected:
            virtual R visitNumberExpr(const Number &expr) = 0;
            virtual R visitVariableExpr(const Variable &expr) = 0;
            virtual R visitBinaryExpr(const Binary &expr) = 0;
            virtual R visitCallExpr(const Call &expr) = 0;
        };
    } // namespace expr

    /** @brief Statement ast */
    namespace statement
    {
        struct Function;
        struct Expression;
        struct Return;

        using FunctionPtr = std::unique_ptr<Function>;
        using ExpressionPtr = std::unique_ptr<Expression>;
        using ReturnPtr = std::unique_ptr<Return>;
        using StmtPtr = std::variant<FunctionPtr, ExpressionPtr, ReturnPtr>;

        struct Function : private Uncopyable
        {
            std::string Name;
            std::vector<std::string> Args;
            std::vector<StmtPtr> Body;

            Function(const std::string &name,
                     std::vector<std::string> args,
                     std::vector<StmtPtr> body)
                : Name{name}, Args{std::move(args)}, Body{std::move(body)} {}
        };

        struct Expression : private Uncopyable
        {
            expression::ExprPtr Expr;

            Expression(expression::ExprPtr expr) : Expr{std::move(expr)} {}
        };

        struct Return : private Uncopyable
        {
            expression::ExprPtr Expr;

            Return(expression::ExprPtr expr) : Expr{std::move(expr)} {}
        };

        template <typename R>
        class Visitor
        {
        public:
            /** @brief Visit statement node */
            R visit(const StmtPtr &stmt)
            {
                return std::visit(
                    overloaded{
                        [this](const FunctionPtr &stmt)
                        {
                            return visitFunctionStmt(*stmt);
                        },
                        [this](const ExpressionPtr &stmt)
                        {
                            return visitExpressionStmt(*stmt);
                        },
                        [this](const ReturnPtr &stmt)
                        {
                            return visitReturnStmt(*stmt);
                        }},
                    stmt);
            }

        protected:
            virtual R visitFunctionStmt(const Function &stmt) = 0;
            virtual R visitExpressionStmt(const Expression &stmt) = 0;
            virtual R visitReturnStmt(const Return &stmt) = 0;
        };
    } // namespace stmt

    using Program = std::vector<statement::StmtPtr>;

} // namespace ast

#endif