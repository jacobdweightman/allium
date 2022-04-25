#include <sstream>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include "LLVMCodeGen/LogInstrumentor.h"

void LogInstrumentor::log(int minimumLogLevel, const std::string &message) {
    BasicBlock *bb = cg.builder.GetInsertBlock();
    Function *f = bb->getParent();
    BasicBlock *logBB = BasicBlock::Create(cg.ctx, "log.impl", f, bb->getNextNode());
    BasicBlock *next = BasicBlock::Create(cg.ctx, "", f, bb->getNextNode());

    GlobalVariable *logLevelPtr = cg.mod.getNamedGlobal("logLevel");
    assert(logLevelPtr && "couldn't find global variable logLevel");
    Value *logLevel = cg.builder.CreateLoad(Type::getInt32Ty(cg.ctx), logLevelPtr, "log.level");
    Constant *threshold = ConstantInt::get(Type::getInt32Ty(cg.ctx), minimumLogLevel);
    logLevel->getType()->dump();
    threshold->getType()->dump();
    Value *cmp = cg.builder.CreateCmp(CmpInst::Predicate::ICMP_SGE, logLevel, threshold);
    cg.builder.CreateCondBr(cmp, logBB, next);

    cg.builder.SetInsertPoint(logBB);
    llvm::FunctionCallee printf = cg.mod.getOrInsertFunction(
        "printf",
        llvm::FunctionType::get(
            llvm::IntegerType::get(cg.ctx, 32),
            { PointerType::getInt8PtrTy(cg.ctx) },
            true));
    llvm::Value *msgValue = cg.builder.CreateGlobalStringPtr(message);
    cg.builder.CreateCall(printf, { msgValue });
    cg.builder.CreateBr(next);

    cg.builder.SetInsertPoint(next);
}

void LogInstrumentor::logEffect(const TypedAST::EffectCtorRef &ec) {
    std::ostringstream message;
    message << "handle effect: " << ec.effectName << "." << ec.ctorName << "\n";
    log(1, message.str());
}

void LogInstrumentor::logSubproof(const TypedAST::PredicateRef &pr) {
    std::stringstream message;
    message << "prove: " << pr << "\n";
    log(2, message.str());
}

void LogInstrumentor::logImplication(const TypedAST::Implication &impl) {
    std::stringstream message;
    message << "  try implication: " << impl << std::endl;
    log(3, message.str());
}
