#ifndef INTERPRETER_BUILTIN_EFFECTS_H
#define INTERPRETER_BUILTIN_EFFECTS_H

#include <vector>

#include "Interpreter/Program.h"
#include "Utils/Generator.h"
#include "Utils/Unit.h"

namespace interpreter {

Generator<Unit> builtinHandlerIO(
    const Program &prog,
    const EffectCtorRef &ecr,
    Context &context,
    HandlerStack &handlers);

}

#endif // INTERPRETER_BUILTIN_EFFECTS_H
