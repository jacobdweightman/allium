#ifndef LLVMCODEGEN_CG_TYPE_H
#define LLVMCODEGEN_CG_TYPE_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Target/TargetMachine.h>
#include <map>

#include "LLVMCodeGen/CGContext.h"
#include "SemAna/TypedAST.h"
#include "SemAna/TypeRecursionAnalysis.h"

using namespace llvm;

std::string unifyFuncName(Name<TypedAST::Type> name);

struct AlliumType {
    /// The LLVM type corresponding to the Allium type.
    Type *irType = nullptr;

    /// The payload types for each of the Allium type's constructors.
    std::vector<Type *> payloadTypes;
};

class TypeGenerator {
    const TypedAST::AST &ast;
    IRBuilder<> &builder;
    LLVMContext &ctx;
    Module &mod;

    TypedAST::TypeRecursionAnalysis typeRecursionAnalysis;

    std::map<Name<TypedAST::Type>, AlliumType> loweredTypes;

public:
    TypeGenerator(CGContext &cgctx):
        ast(cgctx.ast), builder(cgctx.builder), ctx(cgctx.ctx), mod(cgctx.mod),
        typeRecursionAnalysis(ast.types) {}

    /// Returns the identified struct type representing `type` in the IR.
    AlliumType getIRType(const TypedAST::Type &type);

    void lowerAllTypes();
};

#endif // LLVMCODEGEN_CG_TYPE_H
