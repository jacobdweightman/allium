#ifndef INTERPRETER_BUILTIN_EFFECTS_H
#define INTERPRETER_BUILTIN_EFFECTS_H

#include <vector>

#include "Interpreter/Program.h"

namespace interpreter {

void handleDefaultIO(
    const EffectCtorRef &ecr,
    Context &context
);

}

#endif // INTERPRETER_BUILTIN_EFFECTS_H
