#ifndef PARSER_BUILTINS_H
#define PARSER_BUILTINS_H

#include <vector>

#include "Parser/AST.h"

namespace parser {

extern std::vector<Type> builtinTypes;

Optional<Type *> getTypeIfBuiltin(Name<Type> tn);

bool nameIsBuiltinType(Name<Type> tn);

extern std::vector<Effect> builtinEffects;

} // end namespace parser

#endif // SEMANA_BUILTINS_H
