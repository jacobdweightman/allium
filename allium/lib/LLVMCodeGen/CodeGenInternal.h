#include <map>
#include <set>

#include "Utils/TaggedUnion.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

class LLVMCodeGen;

/// Represents a continuation allocated in another stack frame.
///
/// This is useful to represent the continuations which are implicitly passed as
/// arguments to a function.
class NonlocalContinuationFrame {
    LLVMCodeGen *cg;
    Value *thunk;

public:
    NonlocalContinuationFrame(LLVMCodeGen *cg, Value *thunk):
        cg(cg), thunk(thunk) {}

    Value *getThunk() { return thunk; }
    Value *loadFunc();
    Value *loadSuccess();
    Value *loadFailure();

    /// Builds a call to the thunk this continuation frame represents.
    Value *call();
};

// Represents a continuation allocated in the current stack frame.
//
// Since the members are already constructed (they must have been used to
// construct the frame within the same function), they can be accessed without
// inserting additional instructions.
class LocalContinuationFrame {
    LLVMCodeGen *cg;
    AllocaInst *thunk;
    Value *func;
    Value *success;
    Value *failure;

public:
    LocalContinuationFrame(
        LLVMCodeGen *cg,
        Function *next,
        Value *k,
        Value *f
    );

    Value *getThunk() { return thunk; }
    Value *getFunc() { return func; }
    Value *getSuccess() { return success; }
    Value *getFailure() { return failure; }

    /// Builds a call to the thunk this continuation frame represents.
    Value *call();
};

typedef TaggedUnion<
    NonlocalContinuationFrame,
    LocalContinuationFrame
> ContinuationFrameBase;

class ContinuationFrame : public ContinuationFrameBase {
public:
    using ContinuationFrameBase::ContinuationFrameBase;

    ContinuationFrame(LLVMCodeGen *cg, Value *thunk):
        ContinuationFrameBase(NonlocalContinuationFrame(cg, thunk)) {}

    ContinuationFrame(
        LLVMCodeGen *cg,
        Function *next,
        Value *k,
        Value *f
    ): ContinuationFrameBase(LocalContinuationFrame(cg, next, k, f)) {}

    Value *getThunk() {
        return match<Value*>(
        [](NonlocalContinuationFrame &ncf) { return ncf.getThunk(); },
        [](LocalContinuationFrame &lcf) { return lcf.getThunk(); }
        );
    }

    Value *getFunc() {
        return match<Value*>(
        [](NonlocalContinuationFrame &ncf) { return ncf.loadFunc(); },
        [](LocalContinuationFrame &lcf) { return lcf.getFunc(); }
        );
    }

    Value *getSuccess() {
        return match<Value*>(
        [](NonlocalContinuationFrame &ncf) { return ncf.loadSuccess(); },
        [](LocalContinuationFrame &lcf) { return lcf.getSuccess(); }
        );
    }

    Value *getFailure() {
        return match<Value*>(
        [](NonlocalContinuationFrame &ncf) { return ncf.loadFailure(); },
        [](LocalContinuationFrame &lcf) { return lcf.getFailure(); }
        );
    }

    Value *call() {
        return match<Value*>(
        [](NonlocalContinuationFrame &ncf) { return ncf.call(); },
        [](LocalContinuationFrame &lcf) { return lcf.call(); }
        );
    }
};

class LLVMCodeGen {
    friend NonlocalContinuationFrame;
    friend LocalContinuationFrame;

    IRBuilder<> builder;

    const TypedAST::AST *ast;
    std::map<Name<TypedAST::Type>, Type*> loweredTypes;

    // Convenience
    Type * boolean;
    Type * ptr;
    Type * thunkType;

    // Analysis results
    std::set<Name<TypedAST::Type>> recursiveTypes;

    // lowering logic
    Type *lower(const TypedAST::Type &type);
    void lowerAllTypes();

    FunctionType *getPredIRType(const TypedAST::Predicate &pred);
    void buildTruthLiteral(
        const TypedAST::TruthLiteral &tv,
        ContinuationFrame *k,
        ContinuationFrame *f);
    void buildPredicateRef(
        const TypedAST::PredicateRef &name,
        Value *k,
        Value *f);

    Function *lower(
        const TypedAST::Expression &expr,
        const TypedAST::Predicate &pred,
        size_t choicePoint = 0,
        Function *next = nullptr
    );

    Function *lower(const TypedAST::Predicate &pred);

    // Creates a main function which calls the Allium main predicate.
    Function *createMain();

public:
    LLVMContext ctx;
    Module mod;

    LLVMCodeGen(TargetMachine *tm): ctx(), mod("allium", ctx), builder(ctx) {
        // TODO: these members should be const
        boolean = Type::getInt1Ty(ctx);
        ptr = PointerType::get(ctx, 0);
        thunkType = StructType::get(ctx, { ptr, ptr, ptr });
        mod.setDataLayout(tm->createDataLayout());
    }

    void lower(const TypedAST::AST &ast);
};

std::string mangledPredName(
    Name<TypedAST::Predicate> p,
    size_t choicePoint = 0
);
