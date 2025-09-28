#ifndef HYPERTK_RUNTIME_LLVM_HPP
#define HYPERTK_RUNTIME_LLVM_HPP

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>

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
#ifndef ENABLE_BASIC_JIT_COMPILER
#include "llvm/Target/TargetMachine.h"
#endif

namespace hypertk
{
    using ScopeTable = std::unordered_map<std::string, llvm::Value *>;

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
        /** Initialize JIT compiler */
        void initializeJIT();
#else
        /// @brief Initialize AOT compiler
        bool initializeAOT();
#endif
#ifdef ENABLE_BUILTIN_FUNCTIONS
        /// @brief Compile code to .o file
        bool compileToObjectFile(const std::string &outfile);
        void declareBuiltInFunctions();
#endif

    private:
        std::unique_ptr<llvm::LLVMContext> TheContext_ = nullptr;
        std::unique_ptr<llvm::IRBuilder<>> Builder_ = nullptr;
        std::unique_ptr<llvm::Module> TheModule_ = nullptr;
        // std::map<std::string, llvm::AllocaInst *> NamedValues_;
        llvm::ExitOnError ExitOnErr;
#ifdef ENABLE_BASIC_JIT_COMPILER
        std::unique_ptr<HyperTkJIT> TheJIT_ = nullptr;
#else
        /// @brief Format: `<arch><sub>-<vendor>-<sys>-<abi>`
        /// @example x86_64-unknown-linux-gnu
        const std::string TargetTriple_;
        /// @brief Provide a complete machine description of the machine we’re targeting.
        llvm::TargetMachine *TargetMachine_ = nullptr;
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
        std::vector<ScopeTable> scopes_;

    protected:
        using ast::expression::Visitor<llvm::Value *>::visit;
        using ast::statement::Visitor<llvm::Value *>::visit;

        //> statements
        llvm::Value *visitBlockStmt(const ast::statement::Block &stmt);
        llvm::Value *visitVarDeclStmt(const ast::statement::VarDecl &stmt);
        llvm::Value *visitFunctionStmt(const ast::statement::Function &stmt);
        llvm::Value *visitBinOpDefStmt(const ast::statement::BinOpDef &stmt);
        llvm::Value *visitUnaryOpDefStmt(const ast::statement::UnaryOpDef &stmt);
        llvm::Value *visitExpressionStmt(const ast::statement::Expression &stmt);
        llvm::Value *visitReturnStmt(const ast::statement::Return &stmt);
        llvm::Value *visitIfStmt(const ast::statement::If &stmt);
        llvm::Value *visitForStmt(const ast::statement::For &stmt);
        //<

        //> expressions
        llvm::Value *visitNumberExpr(const ast::expression::Number &expr);
        llvm::Value *visitVariableExpr(const ast::expression::Variable &expr);
        llvm::Value *visitBinaryExpr(const ast::expression::Binary &expr);
        llvm::Value *visitUnaryExpr(const ast::expression::Unary &expr);
        llvm::Value *visitConditionalExpr(const ast::expression::Conditional &expr);
        llvm::Value *visitCallExpr(const ast::expression::Call &expr);
        //<

        inline void beginScope();
        inline void endScope();
        inline ScopeTable &currentScope();
        llvm::Value *resolveVariable(const std::string &varName);
        /// @brief Create an alloca instruction in the entry block of the function. This is used for mutable variables etc.
        llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *theFunction,
                                                 llvm::StringRef varName);
        inline void logError(const std::string &msg);
    };
} // namespace codegen

#endif