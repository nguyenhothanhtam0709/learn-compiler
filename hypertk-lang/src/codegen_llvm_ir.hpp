#ifndef HYPERTK_CODEGEN_LLVM_IR_HPP
#define HYPERTK_CODEGEN_LLVM_IR_HPP

#include <memory>
#include <map>
#include <string>

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include "ast.hpp"
#include "common.hpp"

namespace codegen
{
    class CodegenLlvmIr
        : private Uncopyable,
          protected ast::statement::Visitor<llvm::Value *>,
          protected ast::expression::Visitor<llvm::Value *>
    {
    private:
        std::unique_ptr<llvm::LLVMContext> TheContext_;
        std::unique_ptr<llvm::IRBuilder<>> Builder_;
        std::unique_ptr<llvm::Module> TheModule_;
        std::map<std::string, llvm::Value *> NamedValues_;

    public:
        CodegenLlvmIr();

        void printIR(const ast::Program &program);
        llvm::Value *genIR(const ast::Program &program);

    protected:
        using ast::expression::Visitor<llvm::Value *>::visit;
        using ast::statement::Visitor<llvm::Value *>::visit;

        //> statements
        llvm::Value *visitFunctionStmt(const ast::statement::Function &stmt);
        llvm::Value *visitExpressionStmt(const ast::statement::Expression &stmt);
        llvm::Value *visitReturnStmt(const ast::statement::Return &stmt);
        //<

        //> expressions
        llvm::Value *visitNumberExpr(const ast::expression::Number &expr);
        llvm::Value *visitVariableExpr(const ast::expression::Variable &expr);
        llvm::Value *visitBinaryExpr(const ast::expression::Binary &expr);
        llvm::Value *visitCallExpr(const ast::expression::Call &expr);
        //<

        void logError(const std::string &msg);
    };
} // namespace codegen

#endif