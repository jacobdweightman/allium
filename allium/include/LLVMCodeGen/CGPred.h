#ifndef LLVMCODEGEN_CG_PRED_H
#define LLVMCODEGEN_CG_PRED_H

#include "LLVMCodeGen/CGContext.h"
#include "SemAna/TypedAST.h"

std::string mangledPredName(Name<TypedAST::Predicate> p);

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

class PredicateGenerator {
private:
    const TypedAST::AST &ast;
    IRBuilder<> &builder;
    LLVMContext &ctx;
    Module &mod;

    /// Creates the coroutine into which `pred` will be lowered, and sets up the
    /// entry block for an Allium predicate coroutine.
    PredCoroutine createPredicateCoroutine(const TypedAST::Predicate &pred);

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
    void lower(const TypedAST::TruthLiteral &tl, BasicBlock *fail);

    /// Lowers a predicate reference into a coroutine invocation in the current
    /// builder. If the proof fails, execution continues with the fail block.
    void lower(const TypedAST::PredicateRef &pr, BasicBlock *fail);

    /// Recursively lowers a conjunction in the current builder. If the proof
    /// fails, execution continues with the fail block.
    void lower(const TypedAST::Conjunction &conj, BasicBlock *fail);

    /// Recursively lowers a logical expression in the current builder. If the
    /// proof fails, execution continues with the fail block.
    void lower(const TypedAST::Expression &expr, BasicBlock *fail);

public:
    PredicateGenerator(CGContext &cgctx):
        ast(cgctx.ast), builder(cgctx.builder), ctx(cgctx.ctx), mod(cgctx.mod) {}

    /// Lowers an Allium predicate into an LLVM coroutine.
    Function *lower(const TypedAST::Predicate &pred);

    /// Creates a main function which calls the Allium main predicate.
    Function *createMain();

    /// Returns the type of the LLVM coroutine that the given predicate will be
    /// lowered into.
    FunctionType *getPredIRType(const TypedAST::PredicateDecl &pd);
};

#endif // LLVMCODEGEN_CG_PRED_H
