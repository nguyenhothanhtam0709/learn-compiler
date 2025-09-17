#ifndef HYPERTK_AST_HPP
#define HYPERTK_AST_HPP

#include <string>
#include <vector>
#include <variant>
#include <memory>

namespace hypertk
{
    namespace ast
    {
        /** @brief Base ast node */
        struct Node
        {
            Node() = default;
            virtual ~Node() = default;

            Node(Node &&other) noexcept = default;
            Node &operator=(Node &&other) noexcept = default;

            explicit Node(const Node &) = delete;
            Node &operator=(const Node &) = delete;
        };

        /** @brief Expression ast */
        namespace expr
        {
            struct Number;
            struct Variable;
            using Expr = std::variant<Number, Variable>;

            struct Number : Node
            {
                double Val;

                explicit Number(double val) : Val{val} {}
            };

            struct Variable : Node
            {
                std::string Name;

                explicit Variable(const std::string &name) : Name{name} {}
            };

            template <typename R>
            class Visitor
            {
            public:
                /** @brief Visit expression node */
                R visit(const Expr &expr)
                {
                    return std::visit(*this, expr);
                }

            protected:
                virtual R operator()(const Number &expr) = 0;
                virtual R operator()(const Variable &expr) = 0;
            };
        } // namespace expr

        /** @brief Statement ast */
        namespace stmt
        {
            struct Function;
            struct Expr;
            using Stmt = std::variant<Function, Expr>;

            struct Function : Node
            {
                std::string Name;
                std::vector<std::string> Args;
                std::unique_ptr<expr::Expr> Body;

                Function(const std::string &name,
                         std::vector<std::string> args,
                         std::unique_ptr<expr::Expr> body)
                    : Name{name}, Args{std::move(args)}, Body{std::move(body)} {}
            };

            struct Expr : Node
            {
                std::unique_ptr<expr::Expr> Expression;

                Expr(std::unique_ptr<expr::Expr> expression) : Expression{std::move(expression)} {}
            };

            template <typename R>
            class Visitor
            {
            public:
                /** @brief Visit statement node */
                R visit(const Stmt &stmt)
                {
                    return std::visit(*this, stmt);
                }

            protected:
                virtual R operator()(const Function &stmt) = 0;
                virtual R operator()(const Expr &stmt) = 0;
            };
        } // namespace stmt

    } // namespace ast
} // namespace hypertk

#endif