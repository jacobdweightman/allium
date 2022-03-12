#include "SemAna/TypedAST.h"

#include "CodeGenInternal.h"

FunctionType *LLVMCodeGen::getPredIRType(const TypedAST::Predicate &pred) {
    std::vector<Type*> paramTypes;
    for(const TypedAST::Parameter &param : pred.declaration.parameters) {
        paramTypes.push_back(loweredTypes[param.type]);
    }
    Type *success = PointerType::get(ctx, 0);
    paramTypes.push_back(success);
    Type *failure = PointerType::get(ctx, 0);
    paramTypes.push_back(failure);
    return FunctionType::get(boolean, paramTypes, false);
}

LocalContinuationFrame::LocalContinuationFrame(
    LLVMCodeGen *cg,
    Function *next,
    Value *k,
    Value *f
): cg(cg), func(next), success(k), failure(f) {
    thunk = cg->builder.CreateAlloca(cg->thunkType);
    Value *funcp = cg->builder.CreateStructGEP(cg->thunkType, thunk, 0);
    cg->builder.CreateStore(next, funcp);
    Value *kp = cg->builder.CreateStructGEP(cg->thunkType, thunk, 1);
    cg->builder.CreateStore(k, kp);
    Value *fp = cg->builder.CreateStructGEP(cg->thunkType, thunk, 2);
    cg->builder.CreateStore(f, fp);
}

Value *LocalContinuationFrame::call() {
    return cg->builder.CreateCall(
        FunctionType::get(cg->boolean, { cg->ptr, cg->ptr }, false),
        func,
        { success, failure });
}

Value *NonlocalContinuationFrame::loadFunc() {
    Value *p = cg->builder.CreateStructGEP(cg->thunkType, thunk, 0);
    return cg->builder.CreateLoad(cg->ptr, p);
}

Value *NonlocalContinuationFrame::loadSuccess() {
    Value *p = cg->builder.CreateStructGEP(cg->thunkType, thunk, 1);
    return cg->builder.CreateLoad(cg->ptr, p);
}

Value *NonlocalContinuationFrame::loadFailure() {
    Value *p = cg->builder.CreateStructGEP(cg->thunkType, thunk, 2);
    return cg->builder.CreateLoad(cg->ptr, p);
}

Value *NonlocalContinuationFrame::call() {
    Value *func = loadFunc();
    Value *k = loadSuccess();
    Value *f = loadFailure();
    return cg->builder.CreateCall(
        FunctionType::get(cg->boolean, { cg->ptr, cg->ptr }, false),
        func,
        { k, f });
}

void LLVMCodeGen::buildTruthLiteral(
    const TypedAST::TruthLiteral &tv,
    ContinuationFrame *k,
    ContinuationFrame *f
) {
    ContinuationFrame *frame = tv.value ? k : f;
    Value *result = frame->call();
    builder.CreateRet(result);
}

void LLVMCodeGen::buildPredicateRef(
    const TypedAST::PredicateRef &pr,
    Value *k,
    Value *f
) {
    // TODO: does this work regardless of predicate order? For recursive predicates?
    Function *p = mod.getFunction(mangledPredName(pr.name));
    assert(p && "Couldn't find a function corresponding to this predicate!");
    Value *result = builder.CreateCall(
        p->getFunctionType(),
        p,
        { k, f });
    builder.CreateRet(result);
}

Function *LLVMCodeGen::lower(
    const TypedAST::Expression &expr,
    const TypedAST::Predicate &pred,
    size_t choicePoint,
    Function *next // Can this be computed from choicePoint?
) {
    FunctionType *ftype = getPredIRType(pred);
    Function *function = Function::Create(
        ftype,
        Function::ExternalLinkage,
        mangledPredName(pred.declaration.name, choicePoint),
        mod);

    Value *k = function->arg_end() - 2;
    k->setName("k");
    ContinuationFrame kframe(this, k);

    Value *f = function->arg_end() - 1;
    f->setName("f");
    ContinuationFrame fframe(this, f);

    BasicBlock *entry = BasicBlock::Create(ctx, "entry", function);
    builder.SetInsertPoint(entry);

    if(next != nullptr) {
        // Allium checks each implication for a predicate in sequence. Thus, we
        // modify the failure continuation to proceed with the next implication
        // if the current one fails.
        fframe = ContinuationFrame(this, next, k, f);
        f = fframe.getFunc();
    }

    expr.switchOver(
    [&](TypedAST::TruthLiteral &tv) {
        this->buildTruthLiteral(tv, &kframe, &fframe);
    },
    [&](TypedAST::PredicateRef &pr) {
        this->buildPredicateRef(pr, k, f);
    },
    [&](TypedAST::EffectCtorRef &) {
        assert(false && "Not implemented!");
    },
    [&](TypedAST::Conjunction &conj) {
        // TODO: choice point numbering?
        Function *left = lower(conj.getLeft(), pred, choicePoint);
        Function *right = lower(conj.getRight(), pred, choicePoint);

        // Because `lower` can change the insertion point, reset it.
        builder.SetInsertPoint(entry);

        // Allium's depth-first proof strategy requires us to prove the left
        // conjunct first, and continue on to the right only if that succeeds.
        // Thus, call left, with right as the success continuation.
        kframe = ContinuationFrame(this, right, k, f);
        Value *newk = kframe.getThunk();
        Value *result = builder.CreateCall(
            left->getFunctionType(),
            left,
            { newk, f });
        this->builder.CreateRet(result);
    });

    return function;
}

// Note: assumes all types have already been lowered.
Function *LLVMCodeGen::lower(const TypedAST::Predicate &pred) {
    size_t choicePoint = pred.implications.size() - 1; // TODO: choice point numbering
    Function *func = nullptr;

    for(auto impl = pred.implications.rbegin();
        impl != pred.implications.rend();
        impl++) {
        func = lower(impl->body, pred, choicePoint, func);
        choicePoint--;
    }

    return func;
}

std::string mangledPredName(Name<TypedAST::Predicate> p, size_t choicePoint) {
    return "_pred_" + p.string() + "_" + std::to_string(choicePoint);
}
