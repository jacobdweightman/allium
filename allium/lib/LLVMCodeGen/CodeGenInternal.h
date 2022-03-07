#include <map>
#include <set>

#include "Utils/TaggedUnion.h"
#include "SemAna/TypedAST.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

class LLVMCodeGen;

class LogInstrumentor {
private:
    LLVMCodeGen *cg;

public:
    LogInstrumentor(LLVMCodeGen *cg): cg(cg) {}

    void log(const std::string &message);
    void logEffect(const TypedAST::EffectCtorRef &ec);
    void logSubproof(const TypedAST::PredicateRef &pr);
    void logImplication(const TypedAST::Implication &impl);
};

class LLVMCodeGen {
    friend LogInstrumentor;

    IRBuilder<> builder;

    const TypedAST::AST *ast;
    std::map<Name<TypedAST::Type>, Type*> loweredTypes;
    std::map<Name<TypedAST::Predicate>, Function*> loweredPredicates;

    // Analysis results
    std::set<Name<TypedAST::Type>> recursiveTypes;

    // lowering logic
    Type *lower(const TypedAST::Type &type);
    void lowerAllTypes();

    FunctionType *getPredIRType(const TypedAST::PredicateDecl &pd);

    void lower(const TypedAST::TruthLiteral &tl, BasicBlock *fail);
    void lower(const TypedAST::PredicateRef &pr, BasicBlock *fail);
    void lower(const TypedAST::Conjunction &conj, BasicBlock *fail);
    void lower(const TypedAST::Expression &expr, BasicBlock *fail);
    Function *lower(const TypedAST::Predicate &pred);

    // Creates a main function which calls the Allium main predicate.
    Function *createMain();

public:
    LLVMContext ctx;
    Module mod;

    LLVMCodeGen(TargetMachine *tm): ctx(), mod("allium", ctx), builder(ctx) {
        mod.setDataLayout(tm->createDataLayout());
    }

    void lower(const TypedAST::AST &ast);
};

std::string mangledPredName(Name<TypedAST::Predicate> p);
