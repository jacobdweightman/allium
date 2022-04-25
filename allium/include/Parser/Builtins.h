#ifndef PARSER_BUILTINS_H
#define PARSER_BUILTINS_H

#include <set>

#include "Parser/AST.h"

namespace parser {

bool nameIsBuiltinType(Name<Type> tn);

extern std::vector<Effect> builtinEffects;

} // end namespace parser

#endif // SEMANA_BUILTINS_H
