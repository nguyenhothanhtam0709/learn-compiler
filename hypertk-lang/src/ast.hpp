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
        LESS = (int)token::TokenType::LESS,
    };

    /** @brief Expression ast */
    namespace expression
    {
        struct Number;
        struct Variable;
        struct Binary;
        struct Conditional;
        struct Call;

        using NumberPtr = std::unique_ptr<Number>;
        using VariablePtr = std::unique_ptr<Variable>;
        using BinaryPtr = std::unique_ptr<Binary>;
        using ConditionalPtr = std::unique_ptr<Conditional>;
        using CallPtr = std::unique_ptr<Call>;
        using ExprPtr = std::variant<NumberPtr, VariablePtr, BinaryPtr, ConditionalPtr, CallPtr>;

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

        struct Conditional : private Uncopyable
        {
            ExprPtr Cond;
            ExprPtr Then;
            ExprPtr Else;

            Conditional(expression::ExprPtr cond, ExprPtr then_, ExprPtr else_)
                : Cond{std::move(cond)},
                  Then{std::move(then_)},
                  Else{std::move(else_)} {}
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
                        [this](const ConditionalPtr &expr)
                        {
                            return visitConditionalExpr(*expr);
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
            virtual R visitConditionalExpr(const Conditional &expr) = 0;
            virtual R visitCallExpr(const Call &expr) = 0;
        };
    } // namespace expr

    /** @brief Statement ast */
    namespace statement
    {
        struct Function;
        struct Expression;
        struct Return;
        struct If;
        struct For;

        using FunctionPtr = std::unique_ptr<Function>;
        using ExpressionPtr = std::unique_ptr<Expression>;
        using ReturnPtr = std::unique_ptr<Return>;
        using IfPtr = std::unique_ptr<If>;
        using ForPtr = std::unique_ptr<For>;
        using StmtPtr = std::variant<FunctionPtr, ExpressionPtr, ReturnPtr, IfPtr, ForPtr>;

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

        struct If : private Uncopyable
        {
            expression::ExprPtr Cond;
            StmtPtr Then;
            std::optional<StmtPtr> Else;

            If(expression::ExprPtr cond, StmtPtr then_, std::optional<StmtPtr> else_ = std::nullopt)
                : Cond{std::move(cond)},
                  Then{std::move(then_)},
                  Else{else_.has_value() ? std::move(else_) : std::nullopt} {}
        };

        struct For : private Uncopyable
        {
            std::string VarName;
            expression::ExprPtr Start, End, Step;
            StmtPtr Body;

            For(const std::string &varName,
                expression::ExprPtr start,
                expression::ExprPtr end,
                expression::ExprPtr step,
                StmtPtr body)
                : VarName{std::move(varName)},
                  Start{std::move(start)},
                  End{std::move(end)},
                  Step{std::move(step)},
                  Body{std::move(body)} {}
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
                        },
                        [this](const IfPtr &stmt)
                        {
                            return visitIfStmt(*stmt);
                        },
                        [this](const ForPtr &stmt)
                        {
                            return visitForStmt(*stmt);
                        }},
                    stmt);
            }

        protected:
            virtual R visitFunctionStmt(const Function &stmt) = 0;
            virtual R visitExpressionStmt(const Expression &stmt) = 0;
            virtual R visitReturnStmt(const Return &stmt) = 0;
            virtual R visitIfStmt(const If &stmt) = 0;
            virtual R visitForStmt(const For &stmt) = 0;
        };
    } // namespace stmt

    using Program = std::vector<statement::StmtPtr>;

} // namespace ast

#endif