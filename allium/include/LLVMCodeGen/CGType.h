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

class TypeGenerator {
    CGContext &cgctx;
    const TypedAST::AST &ast;
    IRBuilder<> &builder;
    LLVMContext &ctx;
    Module &mod;

    TypedAST::TypeRecursionAnalysis typeRecursionAnalysis;

public:
    TypeGenerator(CGContext &cgctx):
        cgctx(cgctx), ast(cgctx.ast), builder(cgctx.builder), ctx(cgctx.ctx),
        mod(cgctx.mod), typeRecursionAnalysis(ast.types) {}

    /// Returns the identified struct type representing `type` in the IR.
    AlliumType getIRType(const TypedAST::Type &type);

    /// Constructs a function to unify values of `type`.
    Function *buildUnifyFunc(const TypedAST::Type &type, const AlliumType &loweredType);

    void lowerAllTypes();
};

#endif // LLVMCODEGEN_CG_TYPE_H
