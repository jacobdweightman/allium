#include "SemAna/TypeRecursionAnalysis.h"

namespace TypedAST {

bool TypeRecursionAnalysis::immediatelyContains(const Type &a, const Type &b) {
    for(const auto &ctor : a.constructors) {
        for(const CtorParameter &param : ctor.parameters) {
            if(param.type == b.declaration.name)
                return true;
        }
    }

    return false;
}

bool TypeRecursionAnalysis::recursivelyContains(const Type &a, const Type &b) {
    if(immediatelyContains(a, b))
        return true;
    
    for(const auto &ctor : a.constructors) {
        for(const CtorParameter &param : ctor.parameters) {
            auto c = std::find_if(
                    types.begin(),
                    types.end(),
                    [&](const Type &b) { return b.declaration.name == param.type; });
            if(areMutuallyRecursive(*c, b))
                return true;
        }
    }

    return false;
}

} // namespace TypedAST
