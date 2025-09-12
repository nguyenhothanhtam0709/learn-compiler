#include <cstdlib>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>

int main()
{
    llvm::LLVMContext context;
    llvm::Module module("my_module", context);
    llvm::IRBuilder<> builder(context);

    // int main() { return 42; }
    llvm::FunctionType *funcType =
        llvm::FunctionType::get(builder.getInt32Ty(), false);
    llvm::Function *mainFunc =
        llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", module);

    llvm::BasicBlock *entry =
        llvm::BasicBlock::Create(context, "entry", mainFunc);
    builder.SetInsertPoint(entry);
    builder.CreateRet(builder.getInt32(42));

    module.print(llvm::outs(), nullptr);

    return EXIT_SUCCESS;
}