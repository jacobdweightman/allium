#include "SemAna/TypeRecursionAnalysis.h"

namespace TypedAST {

/// Type a immediately contains b iff there is a constructor of a which has an
/// argument of type b. 
static bool immediately_contains(const Type &a, const Type &b) {
    for(const auto &ctor : a.constructors) {
        for(const CtorParameter &param : ctor.parameters) {
            if(param.type == b.declaration.name)
                return true;
        }
    }

    return false;
}

/// Type a recursively contains b iff a value of type a can contain a sub-value
/// of type b.
static bool recursively_contains(
    const std::vector<Type> &types,
    const Type &a,
    const Type &b
) {
    if(immediately_contains(a, b))
        return true;
    
    for(const auto &ctor : a.constructors) {
        for(const CtorParameter &param : ctor.parameters) {
            auto c = std::find_if(
                    types.begin(),
                    types.end(),
                    [&](const Type &b) { return b.declaration.name == param.type; });
            if(recursively_contains(types, *c, b))
                return true;
        }
    }

    return false;
}

std::set<Name<Type>> getRecursiveTypes(const std::vector<Type> &types) {
    std::set<Name<Type>> result;

    for(const Type &type : types) {
        // A type is recursive iff it recursively contains itself.
        if(recursively_contains(types, type, type)) {
            result.insert(type.declaration.name);
        }
    }

    return result;
}

} // namespace TypedAST
