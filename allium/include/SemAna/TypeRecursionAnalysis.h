#ifndef SEMANA_TYPE_RECURSION_ANALYSIS_H
#define SEMANA_TYPE_RECURSION_ANALYSIS_H

#include <set>

#include "TypedAST.h"

namespace TypedAST {

class TypeRecursionAnalysis {
    const std::vector<Type> &types;

    /// Type a immediately contains b iff there is a constructor of a which has
    /// an argument of type b. Note that this is not a symmetric relation.
    bool immediatelyContains(const Type &a, const Type &b);

    /// Type a recursively contains b iff a value of type a can contain a sub-
    /// value of type b. Note that this is not a symmetric relation.
    bool recursivelyContains(const Type &a, const Type &b);

public:
    TypeRecursionAnalysis(const std::vector<Type> &types): types(types) {}

    /// Type a is mutually recursive with type b iff a recursively contains b
    /// and b recursively contains a. Note that this relation is symmetric.
    bool areMutuallyRecursive(const Type &a, const Type &b) {
        return recursivelyContains(a, b) && recursivelyContains(b, a);
    }

    /// A type is recursive if a value of that type can contain a subvalue of
    /// the same type; that is, if it recursively contains itself.
    bool isRecursive(const Type &type) {
        return recursivelyContains(type, type);
    }
};

}

#endif // SEMANA_TYPE_RECURSION_ANALYSIS_H
