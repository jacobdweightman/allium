#include "Interpreter/Program.h"
#include "Utils/Generator.h"
#include "Utils/Unit.h"

#include <map>

namespace interpreter {

BuiltinPredicate getBuiltinPredicateByName(const std::string &name);
std::string getBuiltinPredicateName(BuiltinPredicate bp);

#define BUILTIN(name) \
    Generator<Unit> name(std::vector<RuntimeValue>)

BUILTIN(concat);

#undef BUILTIN

} // end namespace interpreter
