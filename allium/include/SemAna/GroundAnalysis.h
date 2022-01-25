#include <set>

#include "SemAna/StaticError.h"
#include "SemAna/TypedAST.h"

namespace TypedAST {

/// Proves that each "ground" argument in the program is actually ground at
/// runtime, or emits a diagnostic for any arguments for which this cannot be
/// proven.
///
/// Because this is an interprocedural analysis, it operates on a typed AST.
void checkGroundParameters(const AST &ast, ErrorEmitter &error);

}
