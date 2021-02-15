#include <map>

#include "TypedAST.h"

namespace TypedAST {

Scope getVariables(const AST &ast, const PredicateRef pr);

Scope getVariables(const AST &ast, const Expression expr);

}
