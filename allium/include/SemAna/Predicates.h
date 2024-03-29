#ifndef SEMANA_PREDICATE_H
#define SEMANA_PREDICATE_H

#include <map>
#include <vector>

#include "Parser/AST.h"
#include "SemAna/StaticError.h"
#include "SemAna/TypedAST.h"

/// Raises the given AST to a fully type-checked AST, emitting any diagnostics
/// in the process.
TypedAST::AST checkAll(const parser::AST &ast, ErrorEmitter &error);

#endif // SEMANA_PREDICATE_H
