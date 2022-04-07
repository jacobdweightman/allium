#include "LLVMCodeGen/CGType.h"
#include "SemAna/TypedAST.h"
#include "SemAna/TypeRecursionAnalysis.h"

using namespace llvm;

std::string mangledTypeName(Name<TypedAST::Type> name) {
    return name.string();
}

AlliumType TypeGenerator::getIRType(const TypedAST::Type &type) {
    // Don't create multiple identified struct types for the same type!
    if(loweredTypes.contains(type.declaration.name)) {
        return loweredTypes.at(type.declaration.name);
    }

    // The lowered type is an identified struct. It's body will be set
    // once it is known.
    AlliumType loweredType;
    std::string name = mangledTypeName(type.declaration.name);
    StructType *llvmType = StructType::create(ctx, name);
    loweredType.irType = llvmType;

    // If the value is a variable, then the payload is a pointer to the
    // variable's value.
    Type *ptr = PointerType::get(llvmType, 0);
    TypeSize maxPayloadSize = mod.getDataLayout().getTypeAllocSize(ptr);
    Align maxPayloadAlignment = mod.getDataLayout().getPrefTypeAlign(ptr);

    for(const auto &ctor : type.constructors) {
        std::vector<Type*> loweredParameterTypes;
        for(const auto &param : ctor.parameters) {
            const TypedAST::Type &pType = ast.resolveTypeRef(param.type);
            if(typeRecursionAnalysis.areMutuallyRecursive(type, pType)) {
                loweredParameterTypes.push_back(ptr);
            } else {
                Type *loweredPType = getIRType(pType).irType;
                loweredParameterTypes.push_back(loweredPType);
            }
        }

        Type *payloadType = StructType::create(ctx, loweredParameterTypes);
        loweredType.payloadTypes.push_back(payloadType);

        TypeSize payloadSize = mod.getDataLayout()
            .getTypeAllocSize(payloadType);
        Align payloadAlignment = mod.getDataLayout()
            .getPrefTypeAlign(payloadType);

        if(payloadSize > maxPayloadSize) {
            maxPayloadSize = payloadSize;
        }
        if(payloadAlignment > maxPayloadAlignment) {
            maxPayloadAlignment = payloadAlignment;
        }
    }

    // Build the actual type
    Type *i8 = Type::getInt8Ty(ctx);
    llvmType->setBody({ ArrayType::get(i8, maxPayloadSize), i8 });
    return loweredType;
}

void TypeGenerator::lowerAllTypes() {
    for(const auto &type : ast.types) {
        getIRType(type);
    }
}
