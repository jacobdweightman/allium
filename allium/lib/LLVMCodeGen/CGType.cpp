#include "LLVMCodeGen/CGType.h"
#include "SemAna/TypedAST.h"
#include "SemAna/TypeRecursionAnalysis.h"

using namespace llvm;

std::string mangledTypeName(Name<TypedAST::Type> name) {
    return name.string();
}

Type *TypeGenerator::lower(const TypedAST::Type &type) {
    size_t maxPayloadSize = 0;
    for(const auto &ctor : type.constructors) {
        std::vector<Type*> loweredParameterTypes;
        for(const auto &param : ctor.parameters) {
            if(recursiveTypes.contains(param.type)) {
                loweredParameterTypes.push_back(PointerType::get(ctx, 0));
            } else {
                const TypedAST::Type &pType = ast.resolveTypeRef(param.type);
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
    StructType *llvmType = StructType::create(ctx, mangledTypeName(type.declaration.name));
    if(maxPayloadSize == 0) {
        // If there is no payload, then the type is just a union tag.
        llvmType->setBody({ i8 });
    } else {
        // Place the union tag after the payload to potentially save some
        // space when allocating memory.
        llvmType->setBody({ ArrayType::get(i8, maxPayloadSize), i8 });
    }

    mod.getOrInsertGlobal(
        type.declaration.name.string(),
        llvmType);

    return llvmType;
}

void TypeGenerator::lowerAllTypes() {
    recursiveTypes = TypedAST::getRecursiveTypes(ast.types);

    for(const auto &type : ast.types) {
        lower(type);
    }
}
