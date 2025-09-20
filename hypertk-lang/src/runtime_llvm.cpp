#include <memory>
#include <map>
#include <string>
#include <iostream>

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

#include "runtime_llvm.hpp"
#include "ast.hpp"
#include "error.hpp"

namespace hypertk
{
    RuntimeLLVM::RuntimeLLVM()
        : ast::statement::Visitor<llvm::Value *>(),
          ast::expression::Visitor<llvm::Value *>(),
          TheContext_{std::make_unique<llvm::LLVMContext>()},
          Builder_{std::make_unique<llvm::IRBuilder<>>(*TheContext_)},
          TheModule_{std::make_unique<llvm::Module>("HyperTk codegen", *TheContext_)} {}

    void RuntimeLLVM::printIR(const ast::Program &program)
    {
        genIR(program);

        TheModule_->print(llvm::errs(), nullptr);
    }

    llvm::Value *RuntimeLLVM::genIR(const ast::Program &program)
    {
        for (const auto &stmt : program)
            visit(stmt);

        return nullptr;
    }

    //> statements
    llvm::Value *RuntimeLLVM::visitFunctionStmt(
        const ast::statement::Function &stmt)
    {
        llvm::Function *theFunction = TheModule_->getFunction(stmt.Name);
        if (theFunction && !theFunction->empty())
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

        // if (!Builder_->GetInsertBlock()->getTerminator())
        // {
        //     if (theFunction->getReturnType()->isVoidTy())
        //         Builder_->CreateRetVoid();
        //     else
        //     {
        //         // Return 0.0 for double functions without explicit return
        //         Builder_->CreateRet(llvm::ConstantFP::get(*TheContext_, llvm::APFloat(0.0)));
        //     }
        // }

        std::string errMsg;
        llvm::raw_string_ostream errStream(errMsg);
        if (llvm::verifyFunction(*theFunction, &errStream))
        {
            errStream.flush();
            logError(errMsg);

            theFunction->eraseFromParent();
            return nullptr;
        }

        return theFunction;
    }

    llvm::Value *RuntimeLLVM::visitExpressionStmt(
        const ast::statement::Expression &stmt)
    {
        return visit(stmt.Expr);
    }

    llvm::Value *RuntimeLLVM::visitReturnStmt(
        const ast::statement::Return &stmt)
    {
        if (llvm::Value *expr = visit(stmt.Expr))
        {
            return Builder_->CreateRet(expr);
        }

        return nullptr;
    }
    //<

    //> expressions
    llvm::Value *RuntimeLLVM::visitNumberExpr(
        const ast::expression::Number &expr)
    {
        return llvm::ConstantFP::get(*TheContext_, llvm::APFloat(expr.Val));
    }

    llvm::Value *RuntimeLLVM::visitVariableExpr(
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

    llvm::Value *RuntimeLLVM::visitBinaryExpr(
        const ast::expression::Binary &expr)
    {
        llvm::Value *L = visit(expr.LHS);
        if (!L)
            return nullptr;

        llvm::Value *R = visit(expr.RHS);
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

    llvm::Value *RuntimeLLVM::visitCallExpr(
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
        for (unsigned i = 0, e = expr.Args.size(); i != e; ++i)
        {
            auto arg = visit(expr.Args[i]);
            if (!arg)
                return nullptr;
            argsV.push_back(arg);
        }

        return Builder_->CreateCall(calleeF, argsV, "calltmp");
    }
    //<

    void RuntimeLLVM::logError(const std::string &msg)
    {
        error::error(0, msg);
    }
} // namespace codegen
