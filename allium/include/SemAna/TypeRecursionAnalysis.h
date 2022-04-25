#ifndef SEMANA_TYPE_RECURSION_ANALYSIS_H
#define SEMANA_TYPE_RECURSION_ANALYSIS_H

#include <set>

#include "TypedAST.h"

namespace TypedAST {

class TypeRecursionAnalysis {
    const std::vector<Type> &types;

    /// Type a immediately contains b iff there is a constructor of a which has
    /// an argument of type b. 
    bool immediatelyContains(const Type &a, const Type &b);

public:
    TypeRecursionAnalysis(const std::vector<Type> &types): types(types) {}

    /// Type a is mutually recursive with type b iff a value of type a can
    /// contain a sub-value of type b. This is a symmetric relation.
    bool areMutuallyRecursive(const Type &a, const Type &b);

    /// A type is recursive if a value of that type can contain a subvalue of
    /// the same type; that is, if it is mutually recursive with itself.
    bool isRecursive(const Type &type) {
        return areMutuallyRecursive(type, type);
    }
};

}

#endif // SEMANA_TYPE_RECURSION_ANALYSIS_H
