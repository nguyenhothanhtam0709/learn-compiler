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
        // Builtin operators
        ADD = (int)token::TokenType::PLUS,
        SUB = (int)token::TokenType::MINUS,
        MUL = (int)token::TokenType::STAR,
        DIV = (int)token::TokenType::SLASH,
        LESS = (int)token::TokenType::LESS,
        EQUAL = (int)token::TokenType::EQUAL,
        // Operators allow user to define custom behaviour
        GREATER = (int)token::TokenType::GREATER,
        EXCLAMATION = (int)token::TokenType::EXCLAMATION,
        VERTICAL_BAR = (int)token::TokenType::VERTICAL_BAR,
        AMPERSAND = (int)token::TokenType::AMPERSAND,
        COLON = (int)token::TokenType::COLON,
    };

    constexpr char BinaryOp2Char(BinaryOp op)
    {
        switch (op)
        {
        case BinaryOp::ADD:
            return '+';
        case BinaryOp::SUB:
            return '-';
        case BinaryOp::MUL:
            return '*';
        case BinaryOp::DIV:
            return '/';
        case BinaryOp::LESS:
            return '<';
        case BinaryOp::GREATER:
            return '>';
        case BinaryOp::EXCLAMATION:
            return '!';
        case BinaryOp::VERTICAL_BAR:
            return '|';
        case BinaryOp::AMPERSAND:
            return '&';
        case BinaryOp::EQUAL:
            return '=';
        case BinaryOp::COLON:
            return ':';
        default:
            return '\0';
        }
    }

    enum class UnaryOp
    {
        // Operators allow user to define custom behaviour
        MINUS = (int)token::TokenType::MINUS,
        EXCLAMATION = (int)token::TokenType::EXCLAMATION,
    };

    constexpr bool isUnaryOp(token::TokenType t)
    {
        switch (t)
        {
        case token::TokenType::MINUS:
        case token::TokenType::EXCLAMATION:
            return true;
        default:
            return false;
        }
    }

    constexpr char UnaryOp2Char(UnaryOp op)
    {
        switch (op)
        {
        case UnaryOp::MINUS:
            return '-';
        case UnaryOp::EXCLAMATION:
            return '!';
        default:
            return '\0';
        }
    }

    enum class FuncKind
    {
        FUNCTION,
        UNARY_OP,
        BINARY_OP,
    };

    /** @brief Expression ast */
    namespace expression
    {
        struct Number;
        struct Variable;
        struct Binary;
        struct Unary;
        struct Conditional;
        struct Call;

        using NumberPtr = std::unique_ptr<Number>;
        using VariablePtr = std::unique_ptr<Variable>;
        using BinaryPtr = std::unique_ptr<Binary>;
        using UnaryPtr = std::unique_ptr<Unary>;
        using ConditionalPtr = std::unique_ptr<Conditional>;
        using CallPtr = std::unique_ptr<Call>;
        using ExprPtr = std::variant<NumberPtr,
                                     VariablePtr,
                                     BinaryPtr,
                                     UnaryPtr,
                                     ConditionalPtr,
                                     CallPtr>;

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

        struct Unary : private Uncopyable
        {
            UnaryOp Op;
            ExprPtr Operand;

            Unary(UnaryOp op, ExprPtr operand)
                : Op{op}, Operand{std::move(operand)} {}
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
                        [this](const UnaryPtr &expr)
                        {
                            return visitUnaryExpr(*expr);
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
            virtual R visitUnaryExpr(const Unary &expr) = 0;
            virtual R visitConditionalExpr(const Conditional &expr) = 0;
            virtual R visitCallExpr(const Call &expr) = 0;
        };
    } // namespace expr

    /** @brief Statement ast */
    namespace statement
    {
        struct Block;
        struct VarDecl;
        struct Function;
        struct BinOpDef;
        struct UnaryOpDef;
        struct Expression;
        struct Return;
        struct If;
        struct For;

        using BlockPtr = std::unique_ptr<Block>;
        using VarDeclPtr = std::unique_ptr<VarDecl>;
        using FunctionPtr = std::unique_ptr<Function>;
        using BinOpDefPtr = std::unique_ptr<BinOpDef>;
        using UnaryOpDefPtr = std::unique_ptr<UnaryOpDef>;
        using ExpressionPtr = std::unique_ptr<Expression>;
        using ReturnPtr = std::unique_ptr<Return>;
        using IfPtr = std::unique_ptr<If>;
        using ForPtr = std::unique_ptr<For>;
        using StmtPtr = std::variant<BlockPtr,
                                     VarDeclPtr,
                                     FunctionPtr,
                                     BinOpDefPtr,
                                     UnaryOpDefPtr,
                                     ExpressionPtr,
                                     ReturnPtr,
                                     IfPtr,
                                     ForPtr>;

        /// @brief Block statement, a collection of many statements
        struct Block : private Uncopyable
        {
            std::vector<StmtPtr> Statements;

            explicit Block(std::vector<StmtPtr> statements)
                : Statements{std::move(statements)} {}
        };

        /// @brief Variable declaration
        struct VarDecl : private Uncopyable
        {
            std::string VarName;
            std::optional<expression::ExprPtr> Initializer;

            VarDecl(std::string varName,
                    std::optional<expression::ExprPtr> initializer_ = std::nullopt)
                : VarName{std::move(varName)},
                  Initializer{initializer_.has_value()
                                  ? std::move(initializer_)
                                  : std::nullopt} {}
        };

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

        /** @brief define custom binary operator */
        struct BinOpDef : public Function
        {
            unsigned Precedence;

            BinOpDef(const std::string &name,
                     std::vector<std::string> args,
                     std::vector<StmtPtr> body,
                     unsigned prec = 0)
                : Function(std::move(name), std::move(args), std::move(body)), Precedence{prec} {}

            const char getOperator() const
            {
                return Name[Name.size() - 1];
            }
        };

        /** @brief define custom unary operator */
        struct UnaryOpDef : public Function
        {
            UnaryOpDef(const std::string &name,
                       std::vector<std::string> args,
                       std::vector<StmtPtr> body)
                : Function(std::move(name), std::move(args), std::move(body)) {}

            const char getOperator() const
            {
                return Name[Name.size() - 1];
            }
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
                        [this](const BlockPtr &stmt)
                        {
                            return visitBlockStmt(*stmt);
                        },
                        [this](const VarDeclPtr &stmt)
                        {
                            return visitVarDeclStmt(*stmt);
                        },
                        [this](const FunctionPtr &stmt)
                        {
                            return visitFunctionStmt(*stmt);
                        },
                        [this](const BinOpDefPtr &stmt)
                        {
                            return visitBinOpDefStmt(*stmt);
                        },
                        [this](const UnaryOpDefPtr &stmt)
                        {
                            return visitUnaryOpDefStmt(*stmt);
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
            virtual R visitBlockStmt(const Block &stmt) = 0;
            virtual R visitVarDeclStmt(const VarDecl &stmt) = 0;
            virtual R visitFunctionStmt(const Function &stmt) = 0;
            virtual R visitBinOpDefStmt(const BinOpDef &stmt) = 0;
            virtual R visitUnaryOpDefStmt(const UnaryOpDef &stmt) = 0;
            virtual R visitExpressionStmt(const Expression &stmt) = 0;
            virtual R visitReturnStmt(const Return &stmt) = 0;
            virtual R visitIfStmt(const If &stmt) = 0;
            virtual R visitForStmt(const For &stmt) = 0;
        };
    } // namespace stmt

    using Program = std::vector<statement::StmtPtr>;

} // namespace ast

#endif