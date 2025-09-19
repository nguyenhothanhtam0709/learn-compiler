#include <memory>
#include <map>
#include <string>

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Verifier.h"

#include "codegen_llvm_ir.hpp"
#include "ast.hpp"
#include "error.hpp"

namespace codegen
{
    CodegenLlvmIr::CodegenLlvmIr()
        : ast::statement::Visitor<llvm::Value *>(),
          ast::expression::Visitor<llvm::Value *>(),
          TheContext_{std::make_unique<llvm::LLVMContext>()},
          TheModule_{std::make_unique<llvm::Module>("HyperTk codegen", *TheContext_)},
          Builder_{std::make_unique<llvm::IRBuilder<>>(*TheContext_)} {}

    //> statements
    llvm::Value *CodegenLlvmIr::visitFunctionStmt(
        const ast::statement::Function &stmt)
    {
        llvm::Function *theFunction = TheModule_->getFunction(stmt.Name);
        if (!theFunction->empty())
        {
            logError("Function cannot be redefined.");
            return nullptr;
        }

        //> Parse function signature
        std::vector<llvm::Type *> Doubles(stmt.Args.size(), llvm::Type::getDoubleTy(*TheContext_));
        llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext_), Doubles, false);
        theFunction = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, stmt.Name, TheModule_.get());

        // Set argument names
        unsigned idx = 0;
        for (auto &arg : theFunction->args())
            arg.setName(stmt.Args[idx++]);
        //<

        //> Parse function body

        // Create a new basic block to start insertion into.
        // Basic blocks in LLVM are an important part of functions that define the Control Flow Graph.
        llvm::BasicBlock *bB = llvm::BasicBlock::Create(*TheContext_, "entry", theFunction);
        // tells the builder that new instructions should be inserted into the end of the new basic block.
        Builder_->SetInsertPoint(bB);

        // Record the function arguments in the NamedValues map.
        NamedValues_.clear();
        for (auto &arg : theFunction->args())
            NamedValues_[std::string(arg.getName())] = &arg;

        for (const auto &fStmt : stmt.Body)
            visit(fStmt);

        Builder_->CreateRetVoid(); // Create the return statement

        if (llvm::verifyFunction(*theFunction))
        {
            return theFunction;
        }

        return nullptr;
        //<
    }

    llvm::Value *CodegenLlvmIr::visitExpressionStmt(
        const ast::statement::Expression &stmt)
    {
        return visit(stmt.Expr);
    }
    //<

    //> expressions
    llvm::Value *CodegenLlvmIr::visitNumberExpr(
        const ast::expression::Number &expr)
    {
        return llvm::ConstantFP::get(*TheContext_, llvm::APFloat(expr.Val));
    }

    llvm::Value *CodegenLlvmIr::visitVariableExpr(
        const ast::expression::Variable &expr)
    {
        llvm::Value *v = NamedValues_[expr.Name];
        if (!v)
        {
            logError("Unknown variable name");
            return nullptr;
        }

        return v;
    }

    llvm::Value *CodegenLlvmIr::visitBinaryExpr(
        const ast::expression::Binary &expr)
    {
        llvm::Value *L = visit(expr.LHS);
        if (!L)
            return nullptr;

        llvm::Value *R = visit(expr.LHS);
        if (!R)
            return nullptr;

        switch (expr.Op)
        {
        case ast::BinaryOp::ADD:
            return Builder_->CreateFAdd(L, R, "addtmp");
        case ast::BinaryOp::SUB:
            return Builder_->CreateFSub(L, R, "subtmp");
        case ast::BinaryOp::MUL:
            return Builder_->CreateFMul(L, R, "multmp");
        case ast::BinaryOp::DIV:
            return Builder_->CreateFDiv(L, R, "divtmp");
        default:
            logError("Unsupported binary operator.");
            return nullptr;
        }
    }

    llvm::Value *CodegenLlvmIr::visitCallExpr(
        const ast::expression::Call &expr)
    {
        // Look up the name in the global module table.
        llvm::Function *calleeF = TheModule_->getFunction(expr.Callee);
        if (!calleeF)
        {
            logError(std::string("Unknown referenced function [ ") + expr.Callee + " ]");
            return nullptr;
        }

        if (calleeF->arg_size() != expr.Args.size())
        {
            logError(std::string("Expected ") + std::to_string(calleeF->arg_size()) +
                     " arguments, got " + std::to_string(expr.Args.size()) + " arguments");
            return nullptr;
        }

        std::vector<llvm::Value *> argsV;
        for (unsigned i = 0, e = argsV.size(); i != e; ++i)
        {
            auto arg = visit(expr.Args[i]);
            if (!arg)
                return nullptr;
            argsV.push_back(arg);
        }

        return Builder_->CreateCall(calleeF, argsV, "calltmp");
    }
    //<

    void CodegenLlvmIr::logError(const std::string &msg)
    {
        error::error(0, msg);
    }
} // namespace codegen
