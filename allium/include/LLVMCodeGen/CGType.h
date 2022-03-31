#ifndef LLVMCODEGEN_CG_TYPE_H
#define LLVMCODEGEN_CG_TYPE_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Target/TargetMachine.h"

#include "LLVMCodeGen/CGContext.h"
#include "SemAna/TypedAST.h"

using namespace llvm;

std::string mangledTypeName(Name<TypedAST::Type> name);

class TypeGenerator {
    const TypedAST::AST &ast;
    IRBuilder<> &builder;
    LLVMContext &ctx;
    Module &mod;

    // Analysis results
    std::set<Name<TypedAST::Type>> recursiveTypes;

    // lowering logic
    Type *lower(const TypedAST::Type &type);

public:
    TypeGenerator(CGContext &cgctx):
        ast(cgctx.ast), builder(cgctx.builder), ctx(cgctx.ctx), mod(cgctx.mod) {}

    void lowerAllTypes();
};

#endif // LLVMCODEGEN_CG_TYPE_H
