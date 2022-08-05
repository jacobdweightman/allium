#ifndef LLVMCODEGEN_CG_PRED_H
#define LLVMCODEGEN_CG_PRED_H

#include "LLVMCodeGen/CGContext.h"
#include "SemAna/TypedAST.h"

struct PredCoroutine {
    // The function (coroutine) into which a predicate is lowered.
    Function *func = nullptr;

    // The token identifying the coroutine (see: @llvm.coro.id)
    Value *id = nullptr;

    // The handle to the coroutine (see: @llvm.coro.begin)
    Instruction *handle = nullptr;

    // The block containing the last suspend point, which indicates that a
    // predicate has no more witnesses.
    BasicBlock *finalSuspend = nullptr;

    // The block which returns control from the coroutine back to the caller.
    BasicBlock *end = nullptr;

    // The block containing the code to cleanup the coroutine. This can be used
    // to clean up a predicate coroutine before all witnesses are exhausted.
    BasicBlock *cleanup = nullptr;
};

using Scope = std::map<Name<TypedAST::Variable>, Value*>;

class PredicateGenerator {
private:
    CGContext &cg;
    const TypedAST::AST &ast;
    IRBuilder<> &builder;
    LLVMContext &ctx;
    Module &mod;

    /// Creates the coroutine into which `pred` will be lowered, and sets up the
    /// entry block for an Allium predicate coroutine.
    PredCoroutine createPredicateCoroutine(const TypedAST::PredicateDecl &pDecl);

    /// Adds code to set up the coroutine by allocating the frame and creating a
    /// coroutine handle. This should only be called by createPredicateCoroutine.
    void addCoroutineInitialization(PredCoroutine &coro);

    /// Adds code for the final suspend point which indicates that there are no
    /// more witnesses. This should only be called by createPredicateCoroutine.
    void addFinalSuspend(PredCoroutine &coro);

    /// Adds code which returns control to the coroutine's caller. This should
    /// only be called by createPredicateCoroutine.
    void addExitPoint(PredCoroutine &coro);

    /// Adds code which cleans up after the coroutine and frees the coroutine
    /// frame. This should only be called by createPredicateCoroutine.
    void addCleanup(PredCoroutine &coro);

    /// Lowers a truth literal into a no-op or branch instruction the current
    /// builder. If the proof fails, execution continues with the fail block.
    void lower(
        const Scope &scope,
        const TypedAST::TruthLiteral &tl,
        BasicBlock *fail);

    /// Lowers a predicate reference into a coroutine invocation in the current
    /// builder. If the proof fails, execution continues with the fail block.
    void lower(
        const Scope &scope,
        const TypedAST::PredicateRef &pr,
        BasicBlock *fail);

    /// Recursively lowers a conjunction in the current builder. If the proof
    /// fails, execution continues with the fail block.
    void lower(
        const Scope &scope,
        const TypedAST::Conjunction &conj,
        BasicBlock *fail);

    /// Recursively lowers a logical expression in the current builder. If the
    /// proof fails, execution continues with the fail block.
    void lower(
        const Scope &scope,
        const TypedAST::Expression &expr,
        BasicBlock *fail);

    /// Recursively lowers an Allium value into stack-allocated memory. This
    /// consists of an alloca followed by one or more stores to initialize the
    /// value.
    Value *lower(const Scope &scope, AlliumType type, const TypedAST::Value &v);

public:
    PredicateGenerator(CGContext &cg):
        cg(cg), ast(cg.ast), builder(cg.builder), ctx(cg.ctx), mod(cg.mod) {}

    /// Lowers an Allium predicate into an LLVM coroutine.
    Function *lower(const TypedAST::UserPredicate &pred);

    /// Creates a main function which calls the Allium main predicate.
    Function *createMain();

    /// Returns the LLVM type corresponding to the given AST type.
    StructType *getTypeIRType(const Name<TypedAST::Type> &type);

    /// Returns the type of the LLVM coroutine that the given predicate will be
    /// lowered into.
    FunctionType *getPredIRType(const TypedAST::PredicateDecl &pd);
};

#endif // LLVMCODEGEN_CG_PRED_H
