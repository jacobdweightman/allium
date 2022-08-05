#ifndef LLVMCODEGEN_CG_CONTEXT_H
#define LLVMCODEGEN_CG_CONTEXT_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>

#include "SemAna/TypedAST.h"

using namespace llvm;

struct AlliumType {
    /// The Allium type's representation in the typed AST.
    const TypedAST::Type *astType = nullptr;

    /// The LLVM type corresponding to the Allium type.
    Type *irType = nullptr;

    /// The payload types for each of the Allium type's constructors.
    std::vector<Type *> payloadTypes;
};

class CGContext {
public:
    const TypedAST::AST &ast;
    IRBuilder<> builder;
    LLVMContext ctx;
    Module mod;

    bool instrumentWithLogs = false;

    /// Relates a type's name with detailed information about it. This is built
    /// during type lowering, and is fully populated before any predicate
    /// lowering.
    std::map<Name<TypedAST::Type>, AlliumType> loweredTypes;

    CGContext(const TypedAST::AST &ast, TargetMachine *tm):
            ast(ast), ctx(), mod("allium", ctx), builder(ctx) {
        mod.setDataLayout(tm->createDataLayout());
    }
};

// The index of the tag inside of an Allium data structure.
inline unsigned int getTagIndex() { return 0; }

// The index of the payload inside of an Allium data structure.
inline unsigned int getPayloadIndex() { return 2; }

std::string mangledTypeName(Name<TypedAST::Type> name);
std::string unifyFuncName(Name<TypedAST::Type> name);
std::string mangledPredName(Name<TypedAST::Predicate> p);

#endif // LLVMCODEGEN_CG_CONTEXT_H
