#ifndef LLVMCODEGEN_LOG_INSTRUMENTOR_H
#define LLVMCODEGEN_LOG_INSTRUMENTOR_H

#include "LLVMCodeGen/CGContext.h"

class LogInstrumentor {
private:
    CGContext &cg;

public:
    LogInstrumentor(CGContext &cg): cg(cg) {}

    void log(int minimumLogLevel, const std::string &message);
    void logEffect(const TypedAST::EffectCtorRef &ec);
    void logSubproof(const TypedAST::PredicateRef &pr);
    void logImplication(const TypedAST::Implication &impl);
};

#endif // LLVMCODEGEN_LOG_INSTRUMENTOR_H
