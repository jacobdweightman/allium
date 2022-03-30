#include <cstdlib>
#include <map>
#include <set>
#include <cstdio>
#include <vector>

#include "SemAna/TypedAST.h"
#include "LLVMCodeGen/CodeGen.h"

#include "llvm/ADT/APInt.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Transforms/Coroutines.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/Host.h"
// #include "llvm/Support/TargetRegistry.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetOptions.h"

#include "CodeGenInternal.h"

using namespace llvm;

Function *LLVMCodeGen::createMain() {
    Function *main = Function::Create(
        FunctionType::get(IntegerType::get(ctx, 32), {}, false),
        Function::ExternalLinkage,
        "main",
        mod);

    BasicBlock *entry = BasicBlock::Create(ctx, "entry", main);
    BasicBlock *failure = BasicBlock::Create(ctx, "failure", main);
    
    builder.SetInsertPoint(entry);
    lower(TypedAST::PredicateRef("main", {}), failure);

    // This goes into the "success" block created by lower
    builder.CreateRet(ConstantInt::get(ctx, APInt(32, 0, true)));

    builder.SetInsertPoint(failure);
    builder.CreateRet(ConstantInt::get(ctx, APInt(32, 1, true)));

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

    std::cout << "print IR: " << config.printLLVMIR << "\n";
    if(config.printLLVMIR) {
        cg.mod.print(llvm::outs(), nullptr);
    }

    std::string objFileName;
    if(config.outputType == compiler::OutputType::OBJECT) {
        objFileName = config.outputFile;
    } else {
        char objTmpName[L_tmpnam];
        if(!std::tmpnam(objTmpName)) {
            std::cout << "Failed to create temporary object file!" << std::endl;
            exit(2);
        }
        objFileName = std::string(objTmpName);
    }   

    std::error_code ec;
    raw_fd_ostream dest(objFileName, ec);
    if (ec) {
        errs() << "Could not open file: " << ec.message();
        return;
    }

    LoopAnalysisManager lam;
    FunctionAnalysisManager fam;
    CGSCCAnalysisManager cgam;
    ModuleAnalysisManager mam;

    PassBuilder pb;
    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);

    // The default pipeline includes coroutine lowering.
    ModulePassManager mpm = pb.buildPerModuleDefaultPipeline(OptimizationLevel::O1);

    mpm.run(cg.mod, mam);

    // As of LLVM 14, backend code generation only works with the legacy pass
    // manager.
    legacy::PassManager pm;
    if(tm->addPassesToEmitFile(pm, dest, nullptr, CGFT_ObjectFile)) {
        errs() << "TheTargetMachine can't emit a file of this type";
        return;
    }
    pm.run(cg.mod);
    dest.flush();
    // TODO: this presumably only works on MacOS!
    system(
        (std::string("ld64.lld -arch x86_64 -platform_version macos 12.0.0 12.0 -o ") +
        config.outputFile + " " + std::string(objFileName) +
        std::string(" -syslibroot /Library/Developer/CommandLineTools/SDKs/MacOSX12.sdk -lSystem")).c_str());
}
