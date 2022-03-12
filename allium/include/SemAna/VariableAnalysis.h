#include <vector>

#include "TypedAST.h"

namespace TypedAST {

/// Returns the variables and their types which are defined inside of the given
/// implication.
std::vector<Name<Variable>> getVariables(const Implication &impl);

}
