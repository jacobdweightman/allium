#ifndef SEMANA_BUILTINS_H
#define SEMANA_BUILTINS_H

#include <vector>

#include "SemAna/TypedAST.h"

namespace TypedAST {

extern std::vector<Type> builtinTypes;

extern std::vector<Effect> builtinEffects;

} // end namespace TypedAST

#endif // SEMANA_BUILTINS_H