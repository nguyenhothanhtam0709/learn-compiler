#include <memory>
#include <map>
#include <string>
#include <iostream>
#include <variant>

#include "runtime_llvm.hpp"
#include "common.hpp"
#include "ast.hpp"
#include "error.hpp"
#ifdef ENABLE_BASIC_JIT_COMPILER
#include "jit.hpp"
#endif

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#ifdef ENABLE_BASIC_JIT_COMPILER
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#else
#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/LegacyPassManager.h"
#endif
#ifdef ENABLE_COMPILER_OPTIMIZATION_PASS
#include "llvm/IR/PassManager.h"
#include "llvm/IR/PassInstrumentation.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#endif

namespace hypertk
{
    RuntimeLLVM::RuntimeLLVM()
        : ast::statement::Visitor<llvm::Value *>(),
          ast::expression::Visitor<llvm::Value *>()
#ifndef ENABLE_BASIC_JIT_COMPILER
          ,
          TargetTriple_{llvm::sys::getDefaultTargetTriple()}
#endif
    {
    }

    void RuntimeLLVM::printIR()
    {
        TheModule_->print(llvm::errs(), nullptr);
    }

    llvm::Value *RuntimeLLVM::genIR(const ast::Program &program)
    {
        beginScope();
        for (const auto &stmt : program)
            visit(stmt);
        endScope();

        return nullptr;
    }

    void RuntimeLLVM::initializeModuleAndManagers()
    {
        // Open new context and module
        TheContext_ = std::make_unique<llvm::LLVMContext>();
        TheModule_ = std::make_unique<llvm::Module>("HyperTk Runtime", *TheContext_);
#ifdef ENABLE_BASIC_JIT_COMPILER
        TheModule_->setDataLayout(TheJIT_->getDataLayout());
#else
        TheModule_->setDataLayout(TargetMachine_->createDataLayout());
        TheModule_->setTargetTriple(TargetTriple_);
#endif

        // Create a new builder for the module
        Builder_ = std::make_unique<llvm::IRBuilder<>>(*TheContext_);

#ifdef ENABLE_COMPILER_OPTIMIZATION_PASS
        // Create new pass and analysis manager
        TheFPM_ = std::make_unique<llvm::FunctionPassManager>();
        TheLAM_ = std::make_unique<llvm::LoopAnalysisManager>();
        TheFAM_ = std::make_unique<llvm::FunctionAnalysisManager>();
        TheCGAM_ = std::make_unique<llvm::CGSCCAnalysisManager>();
        TheMAM_ = std::make_unique<llvm::ModuleAnalysisManager>();
        ThePIC_ = std::make_unique<llvm::PassInstrumentationCallbacks>();
        TheSI_ = std::make_unique<llvm::StandardInstrumentations>(*TheContext_, /* Debug logging*/ true);
        TheSI_->registerCallbacks(*ThePIC_, TheMAM_.get());

        // Add transformation passes
        // Promote allocas to registers.
        TheFPM_->addPass(llvm::PromotePass());
        // Do simple "peephole" optimizations and bit-twiddling optzns.
        TheFPM_->addPass(llvm::InstCombinePass());
        // Reassociate expressions.
        TheFPM_->addPass(llvm::ReassociatePass());
        // Eliminate Common SubExpressions.
        TheFPM_->addPass(llvm::GVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        TheFPM_->addPass(llvm::SimplifyCFGPass());

        // Register analysis passes used in these transform passes.
        llvm::PassBuilder pBuilder;
        pBuilder.registerModuleAnalyses(*TheMAM_);
        pBuilder.registerFunctionAnalyses(*TheFAM_);
        pBuilder.crossRegisterProxies(*TheLAM_, *TheFAM_, *TheCGAM_, *TheMAM_);
#endif
    }

#ifdef ENABLE_BASIC_JIT_COMPILER
    bool RuntimeLLVM::eval()
    {
        if (!TheJIT_)
        {
            logError("JIT compiler must be initialized first.");
            return false;
        }
        if (!TheModule_)
        {
            logError("Module must be initialized first.");
            return false;
        }

        // Create a ResourceTracker to track JIT'd memory allocated to our
        // anonymous expression -- that way we can free it after executing.
        auto RT = TheJIT_->getMainJITDylib().createResourceTracker();

        auto TSM = llvm::orc::ThreadSafeModule(std::move(TheModule_), std::move(TheContext_));
        ExitOnErr(TheJIT_->addModule(std::move(TSM), RT));
        // initializeModuleAndManagers();

        // Search the JIT for the `main` symbol.
        // HyperTk expect a `main` function.
        auto mainSymbol = ExitOnErr(TheJIT_->lookup("main"));

        /// @details Because the LLVM JIT compiler matches the native platform ABI,
        /// this means that you can just cast the result pointer to a function pointer
        /// of that type and call it directly.
        double (*FP)() = mainSymbol.getAddress().toPtr<double (*)()>();
        std::cout << "Eval " << FP() << "\n";

        return true;
    }

    void RuntimeLLVM::initializeJIT()
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();

        TheJIT_ = ExitOnErr(HyperTkJIT::Create());
    }

#else
    bool RuntimeLLVM::compileToObjectFile(const std::string &outfile)
    {
        std::error_code ec;
        llvm::raw_fd_ostream dest(outfile, ec, llvm::sys::fs::OF_None);
        if (ec)
        {
            llvm::errs() << "Could not open file: " << ec.message();
            return false;
        }

        llvm::legacy::PassManager pass;
        auto fileType = llvm::CodeGenFileType::ObjectFile;
        if (TargetMachine_->addPassesToEmitFile(pass, dest, nullptr, fileType))
        {
            llvm::errs() << "TargetMachine can't emit a file of this type";
            return false;
        }

        pass.run(*TheModule_);
        dest.flush();
        return true;
    }

    bool RuntimeLLVM::initializeAOT()
    {
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();

        std::string error_;
        auto target_ = llvm::TargetRegistry::lookupTarget(TargetTriple_, error_);
        // Print an error and exit if we couldn't find the requested target.
        // This generally occurs if we've forgotten to initialise the
        // TargetRegistry or we have a bogus target triple.
        if (!target_)
        {
            llvm::errs() << error_;
            return false;
        }

        auto cpu_ = "generic";
        auto features_ = "";

        llvm::TargetOptions opt;
        TargetMachine_ = target_->createTargetMachine(TargetTriple_,
                                                      cpu_,
                                                      features_,
                                                      opt,
                                                      llvm::Reloc::PIC_);

        return true;
    }

#endif

#ifdef ENABLE_BUILTIN_FUNCTIONS
    void RuntimeLLVM::declareBuiltInFunctions()
    {
        //> define `double @putchard(double)`
        llvm::FunctionType *putchardFT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext_), llvm::Type::getDoubleTy(*TheContext_), false);
        llvm::Function::Create(putchardFT, llvm::Function::ExternalLinkage, "putchard", TheModule_.get());
        //>

        //> define `double @printd(double)`
        llvm::FunctionType *printdFT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext_), llvm::Type::getDoubleTy(*TheContext_), false);
        llvm::Function::Create(printdFT, llvm::Function::ExternalLinkage, "printd", TheModule_.get());
        //>
    }
#endif

    //> statements
    llvm::Value *RuntimeLLVM::visitBlockStmt(
        const ast::statement::Block &stmt)
    {
        beginScope();
        endScope();
        return nullptr;
    }

    llvm::Value *RuntimeLLVM::visitVarDeclStmt(
        const ast::statement::VarDecl &stmt)
    {
        ScopeTable &curScope = currentScope();
        if (curScope.find(stmt.VarName.lexeme) != curScope.end())
        {
            logError("Already a variable with this name in this scope.");
            return nullptr;
        }

        llvm::Function *theFunction = Builder_->GetInsertBlock()->getParent();

        llvm::Value *initializer = nullptr;
        if (stmt.Initializer.has_value())
            if (initializer = visit(stmt.Initializer.value()), !initializer)
                return nullptr;

        llvm::AllocaInst *alloca_ = createEntryBlockAlloca(theFunction, stmt.VarName.lexeme);
        curScope[stmt.VarName.lexeme] = alloca_;
        if (initializer)
            Builder_->CreateStore(initializer, alloca_);

        return alloca_;
    }

    llvm::Value *RuntimeLLVM::visitFunctionStmt(
        const ast::statement::Function &stmt)
    {
        llvm::Function *theFunction = TheModule_->getFunction(stmt.Name.lexeme);
        if (theFunction && !theFunction->empty())
        {
            logError("Function cannot be redefined.");
            return nullptr;
        }

        std::vector<llvm::Type *> Doubles(stmt.Params.size(), llvm::Type::getDoubleTy(*TheContext_));
        llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext_), Doubles, false);
        theFunction = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, stmt.Name.lexeme, TheModule_.get());

        // Set argument names
        unsigned idx = 0;
        for (auto &arg : theFunction->args())
            arg.setName(stmt.Params[idx++].lexeme);

        // Create a new basic block to start insertion into.
        // Basic blocks in LLVM are an important part of functions that define the Control Flow Graph.
        llvm::BasicBlock *bB = llvm::BasicBlock::Create(*TheContext_, "entry", theFunction);
        // tells the builder that new instructions should be inserted into the end of the new basic block.
        Builder_->SetInsertPoint(bB);

        beginScope();

        // Record the function arguments in the NamedValues map.
        // NamedValues_.clear();
        auto &curScope = currentScope();
        for (auto &arg : theFunction->args())
        {
            // Create an alloca for this variable
            llvm::AllocaInst *alloca_ = createEntryBlockAlloca(theFunction, arg.getName());

            // Store the initial value to the alloca
            Builder_->CreateStore(&arg, alloca_);

            // Add arguments to variable symbol table
            // NamedValues_[std::string(arg.getName())] = alloca_;
            curScope[std::string(arg.getName())] = alloca_;
        }

        for (const auto &fStmt : stmt.Body)
            visit(fStmt);

        if (!Builder_->GetInsertBlock()->getTerminator())
        {
            if (theFunction->getReturnType()->isVoidTy())
                Builder_->CreateRetVoid();
            else
            {
                // Return 0.0 for double functions without explicit return
                Builder_->CreateRet(llvm::ConstantFP::get(*TheContext_, llvm::APFloat(0.0)));
            }
        }

        endScope();

        std::string errMsg;
        llvm::raw_string_ostream errStream(errMsg);
        if (llvm::verifyFunction(*theFunction, &errStream))
        {
            errStream.flush();
            logError(errMsg);

            theFunction->eraseFromParent();
            return nullptr;
        }

#ifdef ENABLE_COMPILER_OPTIMIZATION_PASS
        // Run the optimizer on the function.
        TheFPM_->run(*theFunction, *TheFAM_);
#endif

        return theFunction;
    }

    llvm::Value *RuntimeLLVM::visitBinOpDefStmt(
        const ast::statement::BinOpDef &stmt)
    {
        return visitFunctionStmt(stmt);
    }

    llvm::Value *RuntimeLLVM::visitUnaryOpDefStmt(
        const ast::statement::UnaryOpDef &stmt)
    {
        return visitFunctionStmt(stmt);
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

    /// @details Implement an important SSA operation: the `Phi operation`
    llvm::Value *RuntimeLLVM::visitIfStmt(
        const ast::statement::If &stmt)
    {
        llvm::Value *condV = visit(stmt.Cond);
        if (!condV)
            return nullptr;
        // Convert condition to a bool by comparing non-equal to 0.0
        condV = Builder_->CreateFCmpONE(condV,
                                        llvm::ConstantFP::get(*TheContext_, llvm::APFloat(0.0)),
                                        "ifcond");

        llvm::Function *theFunction = Builder_->GetInsertBlock()->getParent();

        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(*TheContext_, "then", theFunction);
        llvm::BasicBlock *elseBB = stmt.Else.has_value()
                                       ? llvm::BasicBlock::Create(*TheContext_, "else")
                                       : nullptr;
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(*TheContext_, "ifcont");

        if (stmt.Else.has_value())
            Builder_->CreateCondBr(condV, thenBB, elseBB);
        else
            Builder_->CreateCondBr(condV, thenBB, mergeBB);

        //> `then` branch
        Builder_->SetInsertPoint(thenBB);
        llvm::Value *thenV = visit(stmt.Then);
        if (!thenV)
            return nullptr;

        if (!Builder_->GetInsertBlock()->getTerminator())
            Builder_->CreateBr(mergeBB);
        // thenBB = Builder_->GetInsertBlock();
        //<

        //> `else` branch
        if (stmt.Else.has_value())
        {
            theFunction->insert(theFunction->end(), elseBB);
            Builder_->SetInsertPoint(elseBB);

            llvm::Value *elseV = visit(stmt.Else.value());
            if (!elseV)
                return nullptr;

            if (!Builder_->GetInsertBlock()->getTerminator())
                Builder_->CreateBr(mergeBB);
            // elseBB = Builder_->GetInsertBlock();
        }
        //<

        //> merge branch
        theFunction->insert(theFunction->end(), mergeBB);
        Builder_->SetInsertPoint(mergeBB);
        //<

        return condV;
    }

    llvm::Value *RuntimeLLVM::visitForStmt(
        const ast::statement::For &stmt)
    {
        llvm::Function *theFunction = Builder_->GetInsertBlock()->getParent();

        // Create an alloca for the variable in the entry block.
        llvm::AllocaInst *alloca_ = createEntryBlockAlloca(theFunction, stmt.VarName.lexeme);

        // Emit the start code first, without `variable` in scope.
        llvm::Value *startVal = visit(stmt.Start);
        if (!startVal)
            return nullptr;

        // Store the value into the alloca
        Builder_->CreateStore(startVal, alloca_);

        // Make the new basic block for the loop header, inserting after current block.
        llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(*TheContext_, "loop", theFunction);

        // Insert an explicit fall through from the current block to the loopBB
        Builder_->CreateBr(loopBB);

        // Start insertion in loopBB
        Builder_->SetInsertPoint(loopBB);

        beginScope();
        // Within the loop, the variable is defined equal to the PHI node.  If it
        // shadows an existing variable, we have to restore it, so save it now.
        // llvm::AllocaInst *oldVal = NamedValues_[stmt.VarName.lexeme];
        // NamedValues_[stmt.VarName.lexeme] = alloca_;
        currentScope()[stmt.VarName.lexeme] = alloca_;

        // Emit the body of the loop.  This, like any other expr, can change the
        // current BB.  Note that we ignore the value computed by the body, but don't
        // allow an error.
        if (!visit(stmt.Body))
        {
            endScope();
            return nullptr;
        }

        // Emit the step values.
        llvm::Value *stepVal = visit(stmt.Step);
        if (!stepVal)
        {
            endScope();
            return nullptr;
        }

        // Compute the end condition.
        llvm::Value *endCond = visit(stmt.End);
        if (!endCond)
        {
            endScope();
            return nullptr;
        }

        // Reload, increment and restore the alloca. This handles the case where
        // the body of the loop mutates the variable.
        llvm::Value *curVar = Builder_->CreateLoad(alloca_->getAllocatedType(),
                                                   alloca_,
                                                   stmt.VarName.lexeme.c_str());
        llvm::Value *nextVar = Builder_->CreateFAdd(curVar, stepVal, "nextvar");
        Builder_->CreateStore(nextVar, alloca_);

        // Convert condition to a bool by comparing non-equal to 0.0.
        endCond = Builder_->CreateFCmpONE(endCond, llvm::ConstantFP::get(*TheContext_, llvm::APFloat(0.0)), "loopcond");

        // Create the `after loop` block and insert it.
        llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(*TheContext_, "afterloop", theFunction);

        // Insert the conditional branch into the end of loopEndBB
        Builder_->CreateCondBr(endCond, loopBB, afterBB);

        // any new code will be inserted in afterBB;
        Builder_->SetInsertPoint(afterBB);

        // // Restore the unshadowed variable.
        // if (oldVal)
        //     NamedValues_[stmt.VarName.lexeme] = oldVal;
        // else
        //     NamedValues_.erase(stmt.VarName.lexeme);
        endScope();

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
        // llvm::AllocaInst *a = NamedValues_[expr.Name.lexeme];
        llvm::Value *v_ = resolveVariable(expr.Name.lexeme);
        if (!v_)
        {
            logError("Unknown variable name");
            return nullptr;
        }

        if (llvm::AllocaInst *a_ = llvm::dyn_cast<llvm::AllocaInst>(v_))
            // Load value
            return Builder_->CreateLoad(a_->getAllocatedType(),
                                        a_,
                                        expr.Name.lexeme.c_str());

        logError("Wrong type");
        return nullptr;
    }

    llvm::Value *RuntimeLLVM::visitBinaryExpr(
        const ast::expression::Binary &expr)
    {
        // Special case '=' because we don't want to emit the LHS as an expression.
        if (expr.Op == ast::BinaryOp::EQUAL)
        {
            // This assume we're building without RTTI because LLVM builds that way by
            // default. If you build LLVM with RTTI this can be changed to a
            // dynamic_cast for automatic error checking.
            if (!std::holds_alternative<ast::expression::VariablePtr>(expr.LHS))
            {
                logError("destination of '=' must be a variable");
                return nullptr;
            }

            auto LHSE = std::get_if<ast::expression::VariablePtr>(&expr.LHS);
            // llvm::Value *variable = NamedValues_[LHSE->get()->Name.lexeme];
            llvm::Value *variable = resolveVariable(LHSE->get()->Name.lexeme);
            if (!variable)
            {
                logError("Unknown variable name.");
                return nullptr;
            }

            llvm::Value *RHS = visit(expr.RHS);
            if (!RHS)
                return nullptr;

            return Builder_->CreateStore(RHS, variable);
        }

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
        case ast::BinaryOp::LESS:
            L = Builder_->CreateFCmpULT(L, R, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
            return Builder_->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext_), "booltmp");
        default:
            break;
        }

        // If it wasn't a builtin binary operator, it must be a user defined one.
        // Emit a call to it.
        if (llvm::Function *func = TheModule_->getFunction(std::string("binary") + ast::BinaryOp2Char(expr.Op)))
        {
            llvm::Value *ops[2] = {L, R};
            return Builder_->CreateCall(func, ops, "binop");
        }

        logError("Unsupported binary operator.");
        return nullptr;
    }

    llvm::Value *RuntimeLLVM::visitUnaryExpr(
        const ast::expression::Unary &expr)
    {
        llvm::Function *func = TheModule_->getFunction(std::string("unary") + ast::UnaryOp2Char(expr.Op));
        if (!func)
        {
            logError("Unsupported unary operator.");
            return nullptr;
        }

        llvm::Value *operandV = visit(expr.Operand);
        if (!operandV)
            return nullptr;

        return Builder_->CreateCall(func, operandV, "unop");
    }

    /// @details Using SSA `Phi operation`
    llvm::Value *RuntimeLLVM::visitConditionalExpr(
        const ast::expression::Conditional &expr)
    {
        llvm::Value *condV = visit(expr.Cond);
        if (!condV)
            return nullptr;
        // Convert condition to a bool by comparing non-equal to 0.0
        condV = Builder_->CreateFCmpONE(condV,
                                        llvm::ConstantFP::get(*TheContext_, llvm::APFloat(0.0)),
                                        "ifcond");

        llvm::Function *theFunction = Builder_->GetInsertBlock()->getParent();

        // Create blocks for the then and else cases. Insert the 'then' block at the
        // end of the function.
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(*TheContext_, "then", theFunction); // automatically insert the new block into the end of the `theFunction` function.
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(*TheContext_, "else");              // aren’t yet inserted into the function.
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(*TheContext_, "ifcont");           // aren’t yet inserted into the function.

        Builder_->CreateCondBr(condV, thenBB, elseBB); // Emit the conditional branch.

        //> Emit then value.
        Builder_->SetInsertPoint(thenBB);

        llvm::Value *thenV = visit(expr.Then);
        if (!thenV)
            return nullptr;

        Builder_->CreateBr(mergeBB); // create an unconditional branch to the merge block to finish `then` block.
        // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
        thenBB = Builder_->GetInsertBlock();
        //<

        //> Emit else block.
        theFunction->insert(theFunction->end(), elseBB);
        Builder_->SetInsertPoint(elseBB);

        llvm::Value *elseV = visit(expr.Else);
        if (!elseV)
            return nullptr;

        Builder_->CreateBr(mergeBB);
        // codegen of 'Else' can change the current block, update ElseBB for the PHI.
        elseBB = Builder_->GetInsertBlock();
        //<

        //> Emit merge block
        theFunction->insert(theFunction->end(), mergeBB);
        Builder_->SetInsertPoint(mergeBB);
        llvm::PHINode *pn = Builder_->CreatePHI(llvm::Type::getDoubleTy(*TheContext_), 2, "iftmp");
        pn->addIncoming(thenV, thenBB);
        pn->addIncoming(elseV, elseBB);
        //<

        return pn;
    }

    llvm::Value *RuntimeLLVM::visitCallExpr(
        const ast::expression::Call &expr)
    {
        // Look up the name in the global module table.
        llvm::Function *calleeF = TheModule_->getFunction(expr.Callee->Name.lexeme);
        if (!calleeF)
        {
            logError(std::string("Unknown referenced function [ ") + expr.Callee->Name.lexeme + " ]");
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

    __attribute__((always_inline)) inline void RuntimeLLVM::beginScope() { scopes_.emplace_back(); }
    __attribute__((always_inline)) inline void RuntimeLLVM::endScope() { scopes_.pop_back(); }
    __attribute__((always_inline)) inline ScopeTable &RuntimeLLVM::currentScope() { return scopes_.back(); }
    llvm::Value *RuntimeLLVM::resolveVariable(const std::string &varName)
    {
        if (!scopes_.empty())
            for (auto scope = scopes_.crbegin(); scope != scopes_.crend(); ++scope)
            {
                auto it = scope->find(varName);
                if (it != scope->end())
                    return it->second;
            }
        return nullptr;
    }
    llvm::AllocaInst *RuntimeLLVM::createEntryBlockAlloca(
        llvm::Function *theFunction,
        llvm::StringRef varName)
    {
        llvm::IRBuilder<> tmpB(&theFunction->getEntryBlock(),
                               theFunction->getEntryBlock().begin());
        return tmpB.CreateAlloca(llvm::Type::getDoubleTy(*TheContext_),
                                 nullptr,
                                 varName);
    }
    __attribute__((always_inline)) inline void RuntimeLLVM::logError(const std::string &msg)
    {
        error::error(0, msg);
    }
} // namespace codegen
