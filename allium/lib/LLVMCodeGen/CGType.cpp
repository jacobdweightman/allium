#include "SemAna/TypedAST.h"
#include "SemAna/TypeRecursionAnalysis.h"

#include "CodeGenInternal.h"

using namespace llvm;

Type *LLVMCodeGen::lower(const TypedAST::Type &type) {
    if(loweredTypes.contains(type.declaration.name))
        return loweredTypes[type.declaration.name];

    size_t maxPayloadSize = 0;
    for(const auto &ctor : type.constructors) {
        std::vector<Type*> loweredParameterTypes;
        for(const auto &param : ctor.parameters) {
            if(recursiveTypes.contains(param.type)) {
                loweredParameterTypes.push_back(PointerType::get(ctx, 0));
            } else {
                const TypedAST::Type &pType = ast->resolveTypeRef(param.type);
                Type *loweredPType = lower(pType);
                loweredParameterTypes.push_back(loweredPType);
            }
        }
        Type *payloadType = StructType::create(ctx, loweredParameterTypes);
        TypeSize payloadSize = mod.getDataLayout()
            .getTypeAllocSize(payloadType);

        if(payloadSize > maxPayloadSize) {
            maxPayloadSize = payloadSize;
        }
    }

    Type *i8 = Type::getInt8Ty(ctx);
    StructType *llvmType;
    if(maxPayloadSize == 0) {
        // If there is no payload, then the type is just a union tag.
        llvmType = StructType::create(
            { i8 },
            type.declaration.name.string());
    } else {
        // Place the union tag after the payload to potentially save some
        // space when allocating memory.
        llvmType = StructType::create(
            { ArrayType::get(i8, maxPayloadSize), i8 },
            type.declaration.name.string());
    }

    loweredTypes.insert({ type.declaration.name, llvmType });

    mod.getOrInsertGlobal(
        type.declaration.name.string(),
        llvmType);

    return llvmType;
}

void LLVMCodeGen::lowerAllTypes() {
    recursiveTypes = TypedAST::getRecursiveTypes(ast->types);

    for(const auto &type : ast->types) {
        lower(type);
    }
}
