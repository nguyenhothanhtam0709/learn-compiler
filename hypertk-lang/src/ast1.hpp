#ifndef HYPERTK_AST_HPP1
#define HYPERTK_AST_HPP1

#include <string>
#include <memory>
#include <vector>

namespace hypertk
{
    namespace ast1
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
            class Number : public Expr<R>
            {
                double Val;

            public:
                explicit Number(double Val) : Val(Val) {}
                R accept(Visitor<R> &visitor) override
                {
                    return visitor.visitNumberExpr(*this);
                }
            };

            template <typename R>
            class Variable : public Expr<R>
            {
                std::string Name;

            public:
                explicit Variable(const std::string &Name) : Name(Name) {}
                R accept(Visitor<R> &visitor) override
                {
                    return visitor.visitVariableExpr(*this);
                }
            };

            template <typename R>
            class Binary : public Expr<R>
            {
                char Op;
                std::unique_ptr<Expr> LHS, RHS;

            public:
                Binary(char Op,
                       std::unique_ptr<Expr> LHS,
                       std::unique_ptr<Expr> RHS)
                    : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
                R accept(Visitor<R> &visitor) override
                {
                    return visitor.visitBinaryExpr(*this);
                }
            };

            template <typename R>
            class Call : public Expr<R>
            {
                std::string Callee;
                std::vector<std::unique_ptr<Expr>> Args;

            public:
                Call(const std::string &Callee,
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
                virtual R visitNumberExpr(Number<R> &expr) = 0;
                virtual R visitVariableExpr(Variable<R> &expr) = 0;
                virtual R visitBinaryExpr(Binary<R> &expr) = 0;
                virtual R visitCallExpr(Call<R> &expr) = 0;
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