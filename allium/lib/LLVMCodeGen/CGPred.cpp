#include "LLVMCodeGen/CGPred.h"
#include "LLVMCodeGen/LogInstrumentor.h"
#include "SemAna/TypedAST.h"
#include "SemAna/VariableAnalysis.h"

using namespace llvm;

StructType *PredicateGenerator::getTypeIRType(const Name<TypedAST::Type> &type) {
    StructType *irType = StructType::getTypeByName(ctx, mangledTypeName(type));
    assert(irType && "type was not already lowered!");
    return irType;
}

FunctionType *PredicateGenerator::getPredIRType(const TypedAST::PredicateDecl &pred) {
    Type *i8ptr = Type::getInt8PtrTy(ctx, 0);
    std::vector<Type*> paramTypes;
    for(const TypedAST::Parameter &param : pred.parameters) {
        Type *type = getTypeIRType(param.type);
        Type *typePtr = PointerType::get(type, 0);
        paramTypes.push_back(typePtr);
    }
    return FunctionType::get(i8ptr, paramTypes, false);
}

void PredicateGenerator::lower(
    const Scope &scope,
    const TypedAST::TruthLiteral &tl,
    BasicBlock *fail
) {
    if(!tl.value) {
        Instruction *br = builder.CreateBr(fail);

        // A`br` instruction cannot occur in the middle of a basic block. Any
        // remaining code for the implication should go into its own unreachable
        // basic block.
        Function *f = builder.GetInsertBlock()->getParent();
        BasicBlock *dead = BasicBlock::Create(ctx, "dead.code", f);
        builder.SetInsertPoint(dead);
    }
}

void PredicateGenerator::lower(
    const Scope &scope,
    const TypedAST::PredicateRef &pr,
    BasicBlock *fail
) {
    const TypedAST::Predicate &p = ast.resolvePredicateRef(pr);
    FunctionCallee pFunc = mod.getOrInsertFunction(
        mangledPredName(pr.name),
        getPredIRType(p.declaration));

    if(cg.instrumentWithLogs) {
        LogInstrumentor(cg).logSubproof(pr);
    }

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

void PredicateGenerator::lower(
    const Scope &scope,
    const TypedAST::Conjunction &conj,
    BasicBlock *fail
) {
    lower(scope, conj.getLeft(), fail);
    lower(scope, conj.getRight(), fail);
}

void PredicateGenerator::lower(
    const Scope &scope,
    const TypedAST::Expression &expr,
    BasicBlock *fail
) {
    expr.switchOver(
    [&](TypedAST::TruthLiteral &tl) { lower(scope, tl, fail); },
    [&](TypedAST::PredicateRef &pr) { lower(scope, pr, fail); },
    [](TypedAST::EffectCtorRef &ecr) { assert(false && "not implemented!"); },
    [&](TypedAST::Conjunction &conj) { lower(scope, conj, fail); }
    );
}

PredCoroutine PredicateGenerator::createPredicateCoroutine(const TypedAST::Predicate &pred) {
    PredCoroutine coro;

    coro.func = Function::Create(
        getPredIRType(pred.declaration),
        GlobalValue::LinkageTypes::ExternalLinkage,
        mangledPredName(pred.declaration.name),
        mod);
    coro.func->addFnAttr("coroutine.presplit", "0");

    // This initializes coro's id and handle, so it must precede the other
    // calls. It must precede the other basic blocks so that the function's
    // entry point is correct.
    addCoroutineInitialization(coro);

    coro.finalSuspend = BasicBlock::Create(ctx, "final.suspend", coro.func);
    coro.cleanup = BasicBlock::Create(ctx, "cleanup", coro.func);
    coro.end = BasicBlock::Create(ctx, "end", coro.func);

    addFinalSuspend(coro);
    addExitPoint(coro);
    addCleanup(coro);

    return coro;
}

void PredicateGenerator::addCoroutineInitialization(PredCoroutine &coro) {
    Function *coroId = Intrinsic::getDeclaration(&mod, Intrinsic::coro_id);
    Function *coroSize = Intrinsic::getDeclaration(&mod, Intrinsic::coro_size, { IntegerType::get(ctx, 32) });
    Function *coroBegin = Intrinsic::getDeclaration(&mod, Intrinsic::coro_begin);
    
    // entry:
    //   %id = call token @llvm.coro.id(i32 0, i8* null, i8* null, i8* null)
    //   %size = call i32 @llvm.coro.size.i32()
    //   %alloc = call i8* @malloc(i32 %size)
    //   %hdl = call i8* @llvm.coro.begin(token %id, i8* %alloc)
    BasicBlock *entry = BasicBlock::Create(ctx, "entry", coro.func);
    builder.SetInsertPoint(entry);

    PointerType *i8ptr = Type::getInt8PtrTy(ctx);
    coro.id = builder.CreateCall(
        coroId->getFunctionType(),
        coroId,
        {
            ConstantInt::get(ctx, APInt(32, 0, true)),
            ConstantPointerNull::get(i8ptr),
            ConstantPointerNull::get(i8ptr),
            ConstantPointerNull::get(i8ptr),
        },
        "id");

    Value *size = builder.CreateCall(coroSize->getFunctionType(), coroSize, {}, "size");

    FunctionCallee malloc = mod.getOrInsertFunction(
        "malloc",
        FunctionType::get(i8ptr, { IntegerType::get(ctx, 32) }, false));
    Value *alloc = builder.CreateCall(malloc, { size }, "alloc");

    coro.handle = builder.CreateCall(
        coroBegin->getFunctionType(),
        coroBegin,
        { coro.id, alloc },
        "hdl");
}

void PredicateGenerator::addFinalSuspend(PredCoroutine &coro) {
    BasicBlock *trap = BasicBlock::Create(ctx, "trap", coro.func);

    Function *coroSuspend = Intrinsic::getDeclaration(&mod, Intrinsic::coro_suspend);
    Function *trapIntrinsic = Intrinsic::getDeclaration(&mod, Intrinsic::trap);

    // final.suspend:
    //   %x = call i8 @llvm.coro.suspend(token none, i1 true)
    //   switch i8 %x, label %suspend [
    //     i8 0, label %trap
    //     i8 1, label %cleanup
    //   ]
    builder.SetInsertPoint(coro.finalSuspend);
    Value *finalSuspendVal = builder.CreateCall(
        coroSuspend->getFunctionType(),
        coroSuspend,
        { ConstantTokenNone::get(ctx), ConstantInt::getTrue(ctx) });
    SwitchInst *siEnd = builder.CreateSwitch(finalSuspendVal, coro.end, 2);
    siEnd->addCase(ConstantInt::get(ctx, APInt(8, 0)), trap);
    siEnd->addCase(ConstantInt::get(ctx, APInt(8, 1)), coro.cleanup);

    // trap:
    //   call void @llvm.trap()
    //   unreachable
    builder.SetInsertPoint(trap);
    builder.CreateCall(trapIntrinsic->getFunctionType(), trapIntrinsic, {});
    builder.CreateUnreachable();
}

void PredicateGenerator::addExitPoint(PredCoroutine &coro) {
    Function *coroEnd = Intrinsic::getDeclaration(&mod, Intrinsic::coro_end);

    // end:
    //   %2 = call i1 @llvm.coro.end(i8* %hdl, i1 false)
    //   ret i8* %hdl
    builder.SetInsertPoint(coro.end);
    builder.CreateCall(
        coroEnd->getFunctionType(),
        coroEnd,
        { coro.handle, ConstantInt::getFalse(ctx) });
    builder.CreateRet(coro.handle);
}

void PredicateGenerator::addCleanup(PredCoroutine &coro) {
    Function *coroFree = Intrinsic::getDeclaration(&mod, Intrinsic::coro_free);

    // cleanup:
    //   %mem = call i8* @llvm.coro.free(token %id, i8* %hdl)
    //   call void @free(i8* %mem)
    //   br label %end
    builder.SetInsertPoint(coro.cleanup);
    Value *mem = builder.CreateCall(coroFree, { coro.id, coro.handle }, "mem");
    FunctionType *freeTy = FunctionType::get(
        Type::getVoidTy(ctx),
        { Type::getInt8PtrTy(ctx) },
        false);
    FunctionCallee free = mod.getOrInsertFunction("free", freeTy);
    builder.CreateCall(free, { mem });
    builder.CreateBr(coro.end);
}

// Note: assumes all types have already been lowered.
Function *PredicateGenerator::lower(const TypedAST::Predicate &pred) {
    PredCoroutine coro = createPredicateCoroutine(pred);

    Function *coroSuspend = Intrinsic::getDeclaration(&mod, Intrinsic::coro_suspend);

    // Iterate backwards so that we have always already created the "next" basic
    // block before we need to create the switch statement at the end of an
    // implication.
    BasicBlock *nextBB = coro.finalSuspend;
    for(auto impl = pred.implications.rbegin();
            impl != pred.implications.rend(); impl++) {
        BasicBlock *bb = BasicBlock::Create(ctx, "", coro.func, nextBB);
        builder.SetInsertPoint(bb);

        if(cg.instrumentWithLogs) {
            LogInstrumentor(cg).logImplication(*impl);
        }

        // Allocate variables that are local to this implication.
        Scope scope;
        const auto variables = getVariables(ast, *impl);
        for(const auto &variable : variables) {
            StructType *irType = getTypeIRType(variable.second.declaration.name);
            Value *var = builder.CreateAlloca(irType);
            Value *tagPtr = builder.CreateStructGEP(irType, var, getTagIndex());
            builder.CreateStore(ConstantInt::get(ctx, APInt(8, 0)), tagPtr);
            scope.insert({ variable.first, var });
        }

        // Generate code for the implication body. On failure, continue with
        // the next implication.
        lower(scope, impl->body, nextBB);

        // there should be one non-final suspend at the end of each implication,
        // which is used if a witness is found for the implication's body.
        Value *suspendVal = builder.CreateCall(
            coroSuspend->getFunctionType(),
            coroSuspend,
            { ConstantTokenNone::get(ctx), ConstantInt::getFalse(ctx) });
        SwitchInst *si = builder.CreateSwitch(suspendVal, coro.end, 2);
        si->addCase(ConstantInt::get(ctx, APInt(8, 0)), nextBB);
        si->addCase(ConstantInt::get(ctx, APInt(8, 1)), coro.cleanup);

        nextBB = bb;
    }

    // Fallthrough from the entry basic block to the first implication.
    builder.SetInsertPoint(&coro.func->getEntryBlock());
    builder.CreateBr(nextBB);

    return coro.func;
}

Value *PredicateGenerator::lower(
    const Scope &scope,
    Type *irType,
    const TypedAST::Type &astType,
    const TypedAST::Value &v
) {
    return v.match<llvm::Value*>(
    [&](TypedAST::AnonymousVariable av) {
        assert(false && "anonymous variable lowering not implemented");
        return nullptr;
    },
    [&](const TypedAST::Variable &v) {
        return scope.at(v.name);
    },
    [&](const TypedAST::ConstructorRef &cr) {
        assert(cr.arguments.size() == 0 && "ctor argument lowering not implemented");
        // Tags of 0 and 1 are reserved for undefined and pointer values, so
        // constructors start from 2.
        size_t tag = getConstructorIndex(astType, cr) + 2;
        Value *alloc = builder.CreateAlloca(irType);
        Value *tagPtr = builder.CreateStructGEP(irType, alloc, getTagIndex());
        builder.CreateStore(ConstantInt::get(ctx, APInt(8, tag)), tagPtr);
        return alloc;
    },
    [&](const TypedAST::StringLiteral &str) {
        assert(false && "string literal lowering not implemented");
        return nullptr;
    },
    [&](TypedAST::IntegerLiteral x) {
        assert(false && "integer literal lowering not implemented");
        return nullptr;
    });
}

Function *PredicateGenerator::createMain() {
    Function *main = Function::Create(
        FunctionType::get(IntegerType::get(ctx, 32), {}, false),
        Function::ExternalLinkage,
        "main",
        mod);

    BasicBlock *entry = BasicBlock::Create(ctx, "entry", main);
    BasicBlock *failure = BasicBlock::Create(ctx, "failure", main);
    
    builder.SetInsertPoint(entry);
    FunctionType *initTy = FunctionType::get(Type::getVoidTy(ctx), {}, false);
    FunctionCallee init = mod.getOrInsertFunction("allium_init", initTy);
    builder.CreateCall(init, {});
    lower({}, TypedAST::PredicateRef("main", {}), failure);

    // This goes into the "success" block created by lower
    builder.CreateRet(ConstantInt::get(ctx, APInt(32, 0, true)));

    builder.SetInsertPoint(failure);
    builder.CreateRet(ConstantInt::get(ctx, APInt(32, 1, true)));

    return main;
}

std::string mangledPredName(Name<TypedAST::Predicate> p) {
    return "_pred_" + p.string();
}
