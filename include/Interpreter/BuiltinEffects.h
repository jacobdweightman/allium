#include <vector>

#include "Interpreter/Program.h"

void handleDefaultIO(
    const interpreter::EffectCtorRef &ecr,
    std::vector<interpreter::Value> &enclosingVariables
);
