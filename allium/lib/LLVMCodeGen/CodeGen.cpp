#include <cstdlib>
#include <map>
#include <set>
#include <cstdio>
#include <vector>

#include "SemAna/TypedAST.h"
#include "LLVMCodeGen/CodeGen.h"

#include "llvm/ADT/APInt.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetOptions.h"

#include "CodeGenInternal.h"

using namespace llvm;

Function *LLVMCodeGen::createMain() {
    auto retBool = [&](const char *name, bool b) {
        Function *f = Function::Create(
            FunctionType::get(boolean, { ptr, ptr }, false),
            Function::PrivateLinkage,
            name,
            mod);
        BasicBlock *entry = BasicBlock::Create(ctx, "entry", f);
        builder.SetInsertPoint(entry);
        builder.CreateRet(ConstantInt::get(boolean, APInt(1, b)));
        return f;
    };

    auto *retT = retBool("retT", true);
    auto *retF = retBool("retF", false);

    Type *i32 = Type::getInt32Ty(ctx);
    Function *main = Function::Create(
        FunctionType::get(i32, {}, false),
        Function::ExternalLinkage,
        "main",
        mod);
    BasicBlock *entry = BasicBlock::Create(ctx, "entry", main);
    
    BasicBlock *success = BasicBlock::Create(ctx, "success", main);
    builder.SetInsertPoint(success);
    builder.CreateRet(ConstantInt::get(i32, APInt(32, 0, true)));

    BasicBlock *failure = BasicBlock::Create(ctx, "failure", main);
    builder.SetInsertPoint(failure);
    builder.CreateRet(ConstantInt::get(i32, APInt(32, 1, true)));

    builder.SetInsertPoint(entry);
    AllocaInst *k = builder.CreateAlloca(thunkType);
    k->setName("k");
    auto *kfunc = builder.CreateStructGEP(thunkType, k, 0);
    builder.CreateStore(retT, kfunc);
    AllocaInst *f = builder.CreateAlloca(thunkType);
    f->setName("f");
    auto *ffunc = builder.CreateStructGEP(thunkType, f, 0);
    builder.CreateStore(retF, ffunc);

    Function *alliumMain = mod.getFunction(mangledPredName("main"));
    assert(alliumMain && "didn't find main!");
    auto *foundProof = builder.CreateCall(
        alliumMain->getFunctionType(),
        alliumMain,
        { k, f });
    builder.CreateCondBr(foundProof, success, failure);

    return main;
}

void LLVMCodeGen::lower(const TypedAST::AST &ast) {
    this->ast = &ast;
    lowerAllTypes();

    for(const auto &pred : ast.predicates) {
        lower(pred);
    }

    createMain();
}

static TargetMachine *initLLVM() {
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    auto targetTriple = sys::getDefaultTargetTriple();
    std::string error;
    auto target = TargetRegistry::lookupTarget(targetTriple, error);
    if(!target) {
        errs() << error;
        return nullptr;
    }

    llvm::Optional<Reloc::Model> relocModel;
    return target->createTargetMachine(
        targetTriple,
        "generic",
        "",
        {},
        relocModel);
}

void cgProgram(const TypedAST::AST &ast, compiler::Config config) {
    TargetMachine *tm = initLLVM();
    if(!tm) {
        exit(1);
    }

    LLVMCodeGen cg(tm);
    cg.lower(ast);
    // cg.mod.print(llvm::outs(), nullptr);

    char objFileName[L_tmpnam];
    if(!std::tmpnam(objFileName)) {
        std::cout << "Failed to create temporary object file!" << std::endl;
        exit(3);
    }

    std::error_code ec;
    raw_fd_ostream dest(objFileName, ec);
    if (ec) {
        errs() << "Could not open file: " << ec.message();
        return;
    }

    legacy::PassManager pm;
    if(tm->addPassesToEmitFile(pm, dest, nullptr, CGFT_ObjectFile)) {
        errs() << "TheTargetMachine can't emit a file of this type";
        return;
    }
    pm.run(cg.mod);
    dest.flush();
    // TODO: this presumably only works on MacOS!
    system(
        (std::string("ld64.lld -arch x86_64 -platform_version macos 11.0.0 11.0 -o ") +
        config.outputFile + " " + std::string(objFileName) +
        std::string(" -syslibroot /Library/Developer/CommandLineTools/SDKs/MacOSX11.0.sdk -lSystem")).c_str());
    
    outs() << "Wrote " << config.outputFile << "\n";
}
