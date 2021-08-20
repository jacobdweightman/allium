#ifndef INTERPRETER_BUILTIN_EFFECTS_H
#define INTERPRETER_BUILTIN_EFFECTS_H

#include <vector>

#include "Interpreter/Program.h"

void handleDefaultIO(
    const interpreter::EffectCtorRef &ecr,
    std::vector<interpreter::Value> &enclosingVariables
);

#endif // INTERPRETER_BUILTIN_EFFECTS_H
