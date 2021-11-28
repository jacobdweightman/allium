#ifndef SEMANA_TYPE_RECURSION_ANALYSIS_H
#define SEMANA_TYPE_RECURSION_ANALYSIS_H

#include <set>

#include "TypedAST.h"

namespace TypedAST {

/// Given the types in a program, determines which types can recursively
/// contain values of the same type.
std::set<Name<Type>> getRecursiveTypes(const std::vector<Type> &types);

}

#endif // SEMANA_TYPE_RECURSION_ANALYSIS_H
