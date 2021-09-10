#include <map>

#include "TypedAST.h"

namespace TypedAST {

/// Returns the variables and their types which are defined inside of the given
/// implication.
Scope getVariables(const AST &ast, const Implication &impl);

}
