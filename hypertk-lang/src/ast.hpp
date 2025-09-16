#ifndef HYPERTK_AST_HPP
#define HYPERTK_AST_HPP

#include <string>
#include <memory>
#include <vector>

namespace hypertk
{
    namespace ast
    {
        namespace expr
        {
            template <typename R>
            class Visitor;

            template <typename R>
            class Expr
            {
            public:
                virtual ~Expr() = default;
                virtual R accept(Visitor<R> &visitor) = 0;
            };

            template <typename R>
            class NumberExpr : public Expr<R>
            {
                double Val;

            public:
                explicit NumberExpr(double Val) : Val(Val) {}
                R accept(Visitor<R> &visitor) override
                {
                    return visitor.visitNumberExpr(*this);
                }
            };

            template <typename R>
            class VariableExpr : public Expr<R>
            {
                std::string Name;

            public:
                explicit VariableExpr(const std::string &Name) : Name(Name) {}
                R accept(Visitor<R> &visitor) override
                {
                    return visitor.visitVariableExpr(*this);
                }
            };

            template <typename R>
            class BinaryExpr : public Expr<R>
            {
                char Op;
                std::unique_ptr<Expr> LHS, RHS;

            public:
                BinaryExpr(char Op,
                           std::unique_ptr<Expr> LHS,
                           std::unique_ptr<Expr> RHS)
                    : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
                R accept(Visitor<R> &visitor) override
                {
                    return visitor.visitBinaryExpr(*this);
                }
            };

            template <typename R>
            class CallExpr : public Expr<R>
            {
                std::string Callee;
                std::vector<std::unique_ptr<Expr>> Args;

            public:
                CallExpr(const std::string &Callee,
                         std::vector<std::unique_ptr<Expr>> Args)
                    : Callee(Callee), Args(std::move(Args)) {}
                R accept(Visitor<R> &visitor) override
                {
                    return visitor.visitCallExpr(*this);
                }
            };

            template <typename R>
            class Visitor
            {
            public:
                virtual ~Visitor() = default;
                virtual R visitNumberExpr(NumberExpr<R> &expr) = 0;
                virtual R visitVariableExpr(VariableExpr<R> &expr) = 0;
                virtual R visitBinaryExpr(BinaryExpr<R> &expr) = 0;
                virtual R visitCallExpr(CallExpr<R> &expr) = 0;
            };
        } // namespace expr

        namespace stmt
        {
            template <typename R>
            class Visitor;

            template <typename R>
            class Stmt
            {
            public:
                virtual ~Stmt() = default;
                virtual R accept(Visitor<R> &visitor) = 0;
            };

            template <typename R>
            class Visitor
            {
            public:
                virtual ~Visitor() = default;
            };
        } // namespace stmt
    } // namespace ast
} // namespace hypertk

#endif