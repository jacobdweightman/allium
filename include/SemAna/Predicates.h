#ifndef SEMANA_PREDICATE_H
#define SEMANA_PREDICATE_H

#include <vector>
#include <map>

#include "Parser/AST.h"
#include "SemAna/StaticError.h"
#include "SemAna/TypedAST.h"

/// Raises the given AST to a fully type-checked AST, emitting any diagnostics
/// in the process.
TypedAST::AST checkAll(const AST &ast, ErrorEmitter &error);

#endif // SEMANA_PREDICATE_H
