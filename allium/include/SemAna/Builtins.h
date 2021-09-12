#ifndef SEMANA_BUILTINS_H
#define SEMANA_BUILTINS_H

#include <vector>

#include "SemAna/TypedAST.h"

#ifdef _MSC_VER
#define PUBLIC_GLOBAL __declspec(dllimport)
#else
#define PUBLIC_GLOBAL
#endif

namespace TypedAST {

PUBLIC_GLOBAL extern std::vector<Type> builtinTypes;

PUBLIC_GLOBAL extern std::vector<Effect> builtinEffects;

} // end namespace TypedAST

#endif // SEMANA_BUILTINS_H