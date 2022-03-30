#include "SemAna/TypedAST.h"

#include "CodeGenInternal.h"

using namespace llvm;

FunctionType *LLVMCodeGen::getPredIRType(const TypedAST::PredicateDecl &pred) {
    Type *i8ptr = Type::getInt8PtrTy(ctx, 0);
    std::vector<Type*> paramTypes;
    for(const TypedAST::Parameter &param : pred.parameters) {
        paramTypes.push_back(loweredTypes[param.type]);
    }
    return FunctionType::get(i8ptr, paramTypes, false);
}

void LLVMCodeGen::lower(const TypedAST::TruthLiteral &tl, BasicBlock *fail) {
    if(!tl.value) {
        Instruction *br = builder.CreateBr(fail);

        // It seems the coroutine passes don't like `br` instructions in the
        // middle of a basic block. Any remaining code for this implication
        // should go into its own unreachable basic block.
        Function *f = builder.GetInsertBlock()->getParent();
        BasicBlock *dead = BasicBlock::Create(ctx, "dead.code", f);
        builder.SetInsertPoint(dead);
    }
}

void LLVMCodeGen::lower(const TypedAST::PredicateRef &pr, BasicBlock *fail) {

    const TypedAST::Predicate &p = ast->resolvePredicateRef(pr);
    FunctionCallee pFunc = mod.getOrInsertFunction(
        mangledPredName(pr.name),
        getPredIRType(p.declaration));

    Function *f = builder.GetInsertBlock()->getParent();
    BasicBlock *success = BasicBlock::Create(ctx, "", f);

    Value *hdl = builder.CreateCall(pFunc, {}, "hdl");
    Function *coroDone = Intrinsic::getDeclaration(&mod, Intrinsic::coro_done);
    Value *done = builder.CreateCall(
        coroDone->getFunctionType(),
        coroDone,
        { hdl },
        "done");

    builder.CreateCondBr(done, fail, success);
    builder.SetInsertPoint(success);
}

void LLVMCodeGen::lower(const TypedAST::Conjunction &conj, BasicBlock *fail) {
    lower(conj.getLeft(), fail);
    lower(conj.getRight(), fail);
}

void LLVMCodeGen::lower(const TypedAST::Expression &expr, BasicBlock *fail) {
    expr.switchOver(
    [&](TypedAST::TruthLiteral &tl) { lower(tl, fail); },
    [&](TypedAST::PredicateRef &pr) { lower(pr, fail); },
    [](TypedAST::EffectCtorRef &ecr) { assert(false && "not implemented!"); },
    [&](TypedAST::Conjunction &conj) { lower(conj, fail); }
    );
}

// Note: assumes all types have already been lowered.
Function *LLVMCodeGen::lower(const TypedAST::Predicate &pred) {
    Function *func = Function::Create(
        getPredIRType(pred.declaration),
        GlobalValue::LinkageTypes::ExternalLinkage,
        mangledPredName(pred.declaration.name),
        mod);

    Function *coroId = Intrinsic::getDeclaration(&mod, Intrinsic::coro_id);
    Function *coroSize = Intrinsic::getDeclaration(&mod, Intrinsic::coro_size, { IntegerType::get(ctx, 32) });
    Function *coroBegin = Intrinsic::getDeclaration(&mod, Intrinsic::coro_begin);
    Function *coroSuspend = Intrinsic::getDeclaration(&mod, Intrinsic::coro_suspend);
    Function *coroEnd = Intrinsic::getDeclaration(&mod, Intrinsic::coro_end);
    Function *coroFree = Intrinsic::getDeclaration(&mod, Intrinsic::coro_free);

    BasicBlock *entry = BasicBlock::Create(ctx, "entry", func);
    builder.SetInsertPoint(entry);

    func->addFnAttr("coroutine.presplit", "0");
    PointerType *i8ptr = PointerType::get(IntegerType::get(ctx, 8), 0);
    Value *id = builder.CreateCall(
        coroId->getFunctionType(),
        coroId,
        { 
            ConstantInt::get(ctx, APInt(32, 0, true)),
            ConstantPointerNull::get(i8ptr),
            ConstantPointerNull::get(i8ptr),
            ConstantPointerNull::get(i8ptr),
        },
        "id");
    
    // Note: it is also possible to get the size as an i64. Perhaps that is a
    // better choice sometimes?
    Value *size = builder.CreateCall(coroSize->getFunctionType(), coroSize, {}, "size");

    FunctionCallee malloc = mod.getOrInsertFunction(
        "malloc",
        FunctionType::get(i8ptr, { IntegerType::get(ctx, 32) }, false));
    Value *alloc = builder.CreateCall(malloc, { size }, "alloc");

    Instruction *hdl = builder.CreateCall(
        coroBegin->getFunctionType(),
        coroBegin,
        { id, alloc },
        "hdl");

    BasicBlock *end = BasicBlock::Create(ctx, "end", func);
    BasicBlock *cleanup = BasicBlock::Create(ctx, "cleanup", func);
    BasicBlock *suspend = BasicBlock::Create(ctx, "suspend", func);
    BasicBlock *trap = BasicBlock::Create(ctx, "trap", func);

    // Iterate backwards so that we have always already created the "next" basic
    // block before we need to create the switch statement at the end of an
    // implication.
    BasicBlock *nextBB = end;
    for(auto impl = pred.implications.rbegin();
            impl != pred.implications.rend(); impl++) {
        BasicBlock *bb = BasicBlock::Create(ctx, "", func, nextBB);
        builder.SetInsertPoint(bb);

        // Generate code for the implication body. On failure, continue with
        // the next implication.
        lower(impl->body, nextBB);

        // there should be one non-final suspend at the end of each implication,
        // which is used if a witness is found for the implication's body.
        Value *suspendVal = builder.CreateCall(
            coroSuspend->getFunctionType(),
            coroSuspend,
            { ConstantTokenNone::get(ctx), ConstantInt::getFalse(ctx) });
        SwitchInst *si = builder.CreateSwitch(suspendVal, suspend, 2);
        si->addCase(ConstantInt::get(ctx, APInt(8, 0)), nextBB);
        si->addCase(ConstantInt::get(ctx, APInt(8, 1)), cleanup);

        nextBB = bb;
    }

    // Fallthrough from the entry basic block to the first implication.
    builder.SetInsertPoint(entry);
    builder.CreateBr(nextBB);

    // end:
    //   %x = call i8 @llvm.coro.suspend(token none, i1 true)
    //   switch i8 %x, label %suspend [
    //     i8 0, label %trap
    //     i8 1, label %cleanup
    //   ]
    builder.SetInsertPoint(end);
    Value *finalSuspendVal = builder.CreateCall(
        coroSuspend->getFunctionType(),
        coroSuspend,
        { ConstantTokenNone::get(ctx), ConstantInt::getTrue(ctx) });
    SwitchInst *siEnd = builder.CreateSwitch(finalSuspendVal, suspend, 2);
    siEnd->addCase(ConstantInt::get(ctx, APInt(8, 0)), trap);
    siEnd->addCase(ConstantInt::get(ctx, APInt(8, 1)), cleanup);

    // suspend:
    //   %2 = call i1 @llvm.coro.end(i8* %hdl, i1 false)
    //   ret i8* %hdl
    builder.SetInsertPoint(suspend);
    builder.CreateCall(
        coroEnd->getFunctionType(),
        coroEnd,
        { hdl, ConstantInt::getFalse(ctx) });
    builder.CreateRet(hdl);

    // cleanup:
    //   %mem = call i8* @llvm.coro.free(token %id, i8* %hdl)
    //   call void @free(i8* %mem)
    //   br label %suspend
    builder.SetInsertPoint(cleanup);
    Value *mem = builder.CreateCall(coroFree, { id, hdl }, "mem");
    FunctionType *freeTy = FunctionType::get(
        Type::getVoidTy(ctx),
        { Type::getInt8PtrTy(ctx) },
        false);
    FunctionCallee free = mod.getOrInsertFunction("free", freeTy);
    builder.CreateCall(free, { mem });
    builder.CreateBr(suspend);

    // trap:
    //   call void @llvm.trap()
    //   unreachable
    builder.SetInsertPoint(trap);
    Function *trapIntrinsic = Intrinsic::getDeclaration(&mod, Intrinsic::trap);
    builder.CreateCall(trapIntrinsic, {});
    builder.CreateUnreachable();

    return func;
}

std::string mangledPredName(Name<TypedAST::Predicate> p) {
    return "_pred_" + p.string();
}
