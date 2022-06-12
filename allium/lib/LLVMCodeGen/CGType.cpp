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

std::string unifyFuncName(Name<TypedAST::Type> name) {
    return "unify" + mangledTypeName(name);
}

Function *TypeGenerator::buildUnifyFunc(const TypedAST::Type &type, const AlliumType &loweredType) {
    std::string name = mangledTypeName(type.declaration.name);

    Type *i1 = Type::getInt1Ty(ctx);
    Type *loweredTypePtr = PointerType::get(loweredType.irType, 0);
    FunctionType *unifyType = FunctionType::get(i1, { loweredTypePtr, loweredTypePtr }, false);
    Function *func = Function::Create(
        unifyType,
        GlobalValue::LinkageTypes::ExternalLinkage,
        Twine("unify", name),
        mod);

    Argument *x = func->getArg(0);
    x->setName("x");
    Argument *y = func->getArg(1);
    y->setName("y");

    BasicBlock *entry = BasicBlock::Create(ctx, "entry", func);
    BasicBlock *asgX = BasicBlock::Create(ctx, "asg.x", func);
    BasicBlock *asgY = BasicBlock::Create(ctx, "asg.y", func);
    BasicBlock *retTrue = BasicBlock::Create(ctx, "ret.true", func);
    BasicBlock *retFalse = BasicBlock::Create(ctx, "ret.false", func);
    BasicBlock *trap = BasicBlock::Create(ctx, "trap", func);

    IntegerType *i8 = Type::getInt8Ty(ctx);
    Type *i8Ptr = Type::getInt8PtrTy(ctx);
    FunctionType *getValueFuncType = FunctionType::get(i8Ptr, { i8Ptr }, false);
    FunctionCallee getValueFunc = mod.getOrInsertFunction("__allium_get_value", getValueFuncType);

    // entry:
    //   %x.val = call T* @__allium_get_value(T* %x)
    //   %x.idx.ptr = getelementptr inbounds T, T* %x.val, i32 0, i32 0
    //   %x.idx = load i8, i8* %x.idx.ptr
    //   %y.val = call T* @getValue(T* %y)
    //   %y.idx.ptr = getelementptr inbounds T, T* %y.val, i32 0, i32 0
    //   %y.idx = load i8, i8* %y.idx.ptr
    //   %y.idx.is.zero = icmp eq i8 %y.idx, 0
    //   br i1 %y.idx.is.zero, label %asg.y, label %switch
    // switch: 
    //   switch i8 %x.idx, label %trap [
    //     i8 0, label %asg.x,
    //     i8 2, label %ctor.0
    //     ...
    //     i8 n+2, label %ctor.n
    //   ]
    builder.SetInsertPoint(entry);
    Value *xI8 = builder.CreatePointerCast(x, i8Ptr, "x.ptr.i8");
    Value *xValI8 = builder.CreateCall(getValueFunc, { xI8 }, "x.val.i8");
    Value *xVal = builder.CreatePointerCast(xValI8, loweredTypePtr, "x.val");
    Value *xIdxPtr = builder.CreateStructGEP(loweredType.irType, xVal, 1, "x.idx.ptr");
    Value *xIdx = builder.CreateLoad(i8, xIdxPtr, "x.idx");

    Value *yI8 = builder.CreatePointerCast(y, i8Ptr, "y.ptr.i8");
    Value *yValI8 = builder.CreateCall(getValueFunc, { yI8 }, "y.val.i8");
    Value *yVal = builder.CreatePointerCast(yValI8, loweredTypePtr, "y.val");
    Value *yIdxPtr = builder.CreateStructGEP(loweredType.irType, yVal, 1, "y.idx.ptr");
    Value *yIdx = builder.CreateLoad(i8, yIdxPtr, "y.idx");

    Value *yIdxIsZero = builder.CreateCmp(
        CmpInst::Predicate::ICMP_EQ,
        yIdx,
        ConstantInt::get(i8, 0),
        "y.idx.is.zero");
    BasicBlock *next = BasicBlock::Create(ctx, "switch", func, asgX);
    builder.CreateCondBr(yIdxIsZero, asgY, next);

    builder.SetInsertPoint(next);
    SwitchInst *si = builder.CreateSwitch(xIdx, trap, type.constructors.size()+1);
    si->addCase(ConstantInt::get(i8, 0), asgX);

    // asg.x:
    //   ; call void @record(%Transaction* %t, T* %x.val)
    //   store i8 1, i8* %x.idx.ptr
    //   %x.payload.ptr = getelementptr inbounds T, T* %x.val, i32 0, i32 1
    //   store T* %y.val, T** %x.payload.ptr
    //   ret i1 1
    builder.SetInsertPoint(asgX);
    builder.CreateStore(ConstantInt::get(i8, 1), xIdxPtr);
    Value *xPayloadPtrI8 = builder.CreateStructGEP(loweredType.irType, xVal, 0, "x.payload.ptr");
    Value *xPayloadPtr = builder.CreatePointerCast(
        xPayloadPtrI8,
        PointerType::get(loweredTypePtr, 0),
        Twine("x.payload.ptr.", name));
    builder.CreateStore(yVal, xPayloadPtr);
    builder.CreateRet(ConstantInt::get(i1, 1));

    // asg.y:
    //   ; call void @record(%Transaction* %t, T* %y.val)
    //   store i8 1, i8* %y.idx.ptr
    //   %y.payload.ptr = getelementptr inbounds T, T* %y.val, i32 0, i32 1
    //   store T* %x.val, T** %y.payload.ptr
    //   ret i1 1
    builder.SetInsertPoint(asgY);
    builder.CreateStore(ConstantInt::get(i8, 1), yIdxPtr);
    Value *yPayloadPtrI8 = builder.CreateStructGEP(loweredType.irType, yVal, 0, "y.payload.ptr");
    Value *yPayloadPtr = builder.CreatePointerCast(
        yPayloadPtrI8,
        PointerType::get(loweredTypePtr, 0),
        Twine("y.payload.ptr.", name));
    builder.CreateStore(xVal, yPayloadPtr);
    builder.CreateRet(ConstantInt::get(i1, 1)); 

    for(int i=0; i<type.constructors.size(); ++i) {
        std::string bbPrefix = "ctor." + std::to_string(i);
        BasicBlock *bb = BasicBlock::Create(ctx, bbPrefix, func, retFalse);
        // Add case to the switch at the end of the entry block
        si->addCase(ConstantInt::get(i8, i+2), bb);

        // At this point, neither value should be undefined (tag = 0) or
        // a variable (tag = 1), so we must be unifying constructors.
        // ctor.n.:
        //   %x.payload.ptr = getelementptr inbounds T, T* %x.val, i32 0, i32 1
        //   %y.payload.ptr = getelementptr inbounds T, T* %y.val, i32 0, i32 1
        //   %ctors.are.eq = icmp eq i8 %x.idx, i8 %y.idx
        //   br i1 %ctors.are.eq, label %asg.y, label %ret.false
        builder.SetInsertPoint(bb);
        Type *payloadType = loweredType.payloadTypes[i];
        Type *payloatTypePtr = PointerType::get(payloadType, 0);
        Value *xPayloadPtrI8 = builder.CreateStructGEP(loweredType.irType, xVal, 0, "x.payload.ptr.i8");
        Value *xPayloadPtr = builder.CreatePointerCast(xPayloadPtrI8, payloatTypePtr, "x.payload.ptr");
        Value *yPayloadPtrI8 = builder.CreateStructGEP(loweredType.irType, yVal, 0, "y.payload.ptr.i8");
        Value *yPayloadPtr = builder.CreatePointerCast(yPayloadPtrI8, payloatTypePtr, "y.payload.ptr");

        Value *ctorsAreEq = builder.CreateCmp(CmpInst::Predicate::ICMP_EQ, xIdx, yIdx, "ctors.are.eq");

        if(type.constructors[i].parameters.size() > 0) {
            bb = BasicBlock::Create(ctx, Twine(bbPrefix, ".0"), func, retTrue);
            builder.CreateCondBr(ctorsAreEq, bb, retFalse);

            for(int j=0; j<type.constructors[i].parameters.size(); ++j) {
                // ctors.i.j:
                //   %arg.x = getelementptr inbounds PayloadT, PayloadT* %x.payload.ptr, i32 0, i32 j
                //   %arg.y = getelementptr inbounds PayloadT, PayloadT* %y.payload.ptr, i32 0, i32 j
                //   %args.match = call i1 @unifyArgType()
                BasicBlock *argBB = BasicBlock::Create(
                    ctx,
                    bbPrefix + std::to_string(j),
                    func, retTrue);
                builder.SetInsertPoint(argBB);
                Type *payloadType = loweredType.payloadTypes[j];
                Type *argType = payloadType->getStructElementType(j);
                Type *argTypePtr = PointerType::get(argType, 0);
                Value *argX = builder.CreateStructGEP(payloadType, xPayloadPtr, j, "arg.x");
                Value *argY = builder.CreateStructGEP(payloadType, yPayloadPtr, j, "arg.y");
                FunctionCallee argUnify = mod.getOrInsertFunction(
                    unifyFuncName(type.constructors[i].parameters[j].type),
                    FunctionType::get(i1, { argTypePtr, argTypePtr }, false));
                builder.CreateCall(argUnify, { argX, argY }, "args.match");
            }
        } else {
            builder.CreateCondBr(ctorsAreEq, retTrue, retFalse);
        }
        // TODO: unify arguments
    }

    // ret.true:
    //   ret i1 true
    builder.SetInsertPoint(retTrue);
    builder.CreateRet(ConstantInt::getTrue(i1));

    // ret.false:
    //   ret i1 false
    builder.SetInsertPoint(retFalse);
    builder.CreateRet(ConstantInt::getFalse(i1));

    // trap:
    //   call void @llvm.trap()
    //   unreachable
    builder.SetInsertPoint(trap);
    Function *trapIntrinsic = Intrinsic::getDeclaration(&mod, Intrinsic::trap);
    builder.CreateCall(trapIntrinsic->getFunctionType(), trapIntrinsic, {});
    builder.CreateUnreachable();

    return func;
}

void TypeGenerator::lowerAllTypes() {
    for(const auto &type : ast.types) {
        AlliumType loweredType = getIRType(type);
        buildUnifyFunc(type, loweredType);
    }
}
