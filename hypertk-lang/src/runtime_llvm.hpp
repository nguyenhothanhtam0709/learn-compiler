#ifndef HYPERTK_RUNTIME_LLVM_HPP
#define HYPERTK_RUNTIME_LLVM_HPP

#include <memory>
#include <map>
#include <string>

#include "common.hpp"
#include "ast.hpp"
#ifdef ENABLE_BASIC_JIT_COMPILER
#include "jit.hpp"
#endif

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Error.h"
#ifdef ENABLE_COMPILER_OPTIMIZATION_PASS
#include "llvm/IR/PassManager.h"
#include "llvm/IR/PassInstrumentation.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#endif

namespace hypertk
{
    class RuntimeLLVM
        : private Uncopyable,
          protected ast::statement::Visitor<llvm::Value *>,
          protected ast::expression::Visitor<llvm::Value *>
    {
    public:
        RuntimeLLVM();

        /** @brief Print LLVM IR */
        void printIR();
        /** @brief Compile AST to LLVM IR */
        llvm::Value *genIR(const ast::Program &program);

        /** @brief Initialize module, compiler pass, ... */
        void initializeModuleAndManagers();
#ifdef ENABLE_BASIC_JIT_COMPILER
        /** Eval the program */
        bool eval();
        /** Enable JIT compiler */
        void initializeJIT();
#endif
#ifdef ENABLE_BUILTIN_FUNCTIONS
        void declareBuiltInFunctions();
#endif

    private:
        std::unique_ptr<llvm::LLVMContext> TheContext_ = nullptr;
        std::unique_ptr<llvm::IRBuilder<>> Builder_ = nullptr;
        std::unique_ptr<llvm::Module> TheModule_ = nullptr;
        std::map<std::string, llvm::Value *> NamedValues_;
        llvm::ExitOnError ExitOnErr;
#ifdef ENABLE_BASIC_JIT_COMPILER
        std::unique_ptr<HyperTkJIT> TheJIT_ = nullptr;
#endif
#ifdef ENABLE_COMPILER_OPTIMIZATION_PASS
        /** @brief the function pass manager */
        std::unique_ptr<llvm::FunctionPassManager> TheFPM_ = nullptr;
        /** @brief the loop analysis manager */
        std::unique_ptr<llvm::LoopAnalysisManager> TheLAM_ = nullptr;
        /** @brief function analysis manager */
        std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM_ = nullptr;
        /** @brief */
        std::unique_ptr<llvm::CGSCCAnalysisManager> TheCGAM_ = nullptr;
        /** @brief Module analysis manager */
        std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM_ = nullptr;
        /** @brief pass instrumentation callbacks */
        std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC_ = nullptr;
        /** @brief standard instrumentation */
        std::unique_ptr<llvm::StandardInstrumentations> TheSI_ = nullptr;
#endif

    protected:
        using ast::expression::Visitor<llvm::Value *>::visit;
        using ast::statement::Visitor<llvm::Value *>::visit;

        //> statements
        llvm::Value *visitFunctionStmt(const ast::statement::Function &stmt);
        llvm::Value *visitExpressionStmt(const ast::statement::Expression &stmt);
        llvm::Value *visitReturnStmt(const ast::statement::Return &stmt);
        llvm::Value *visitIfStmt(const ast::statement::If &stmt);
        llvm::Value *visitForStmt(const ast::statement::For &tmt);
        //<

        //> expressions
        llvm::Value *visitNumberExpr(const ast::expression::Number &expr);
        llvm::Value *visitVariableExpr(const ast::expression::Variable &expr);
        llvm::Value *visitBinaryExpr(const ast::expression::Binary &expr);
        llvm::Value *visitConditionalExpr(const ast::expression::Conditional &expr);
        llvm::Value *visitCallExpr(const ast::expression::Call &expr);
        //<

        void logError(const std::string &msg);
    };
} // namespace codegen

#endif