#include <vector>

#include "Interpreter/Program.h"

using namespace interpreter;

static void printStringValue(
    const Value &v,
    std::vector<Value> &enclosingVariables
) {
    v.switchOver(
    [](ConstructorRef) { assert(false && "IO.print expects a String!"); },
    [](String str) { std::cout << str << "\n"; },
    [](Int i) { assert(false && "IO.print expects a String!"); },
    [&](Value *v) { printStringValue(*v, enclosingVariables); },
    [&](VariableRef vr) {
        assert(!vr.isDefinition);
        printStringValue(enclosingVariables[vr.index], enclosingVariables);
    });
}

void handleDefaultIO(
    const EffectCtorRef &ecr,
    std::vector<Value> &enclosingVariables
) {
    if(ecr.effectCtorIndex == 0) {
        // IO.print(String)
        printStringValue(ecr.arguments[0], enclosingVariables);
    }
}
