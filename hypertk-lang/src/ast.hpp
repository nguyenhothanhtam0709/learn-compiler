#ifndef HYPERTK_AST_HPP
#define HYPERTK_AST_HPP

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
            class Visitor
            {
            public:
                virtual ~Visitor() = default;
                virtual R visitNumberExpr(NumberExpr<R> &expr) = 0;
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