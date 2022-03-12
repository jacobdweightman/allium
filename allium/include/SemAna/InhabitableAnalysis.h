#include <set>

#include "TypedAST.h"

namespace TypedAST {

/// Given the types in a program, determines which types are possible to
/// instantiate.
std::set<Name<Type>> getInhabitableTypes(const std::vector<Type> &types);

}
