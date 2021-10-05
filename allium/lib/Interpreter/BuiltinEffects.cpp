#include <vector>

#include "Interpreter/Program.h"

namespace interpreter {

static void printStringValue(const RuntimeValue &v) {
    v.switchOver(
        [](std::monostate&) { assert(false && "Argument to print must be ground!"); },
        [](RuntimeCtorRef&) { assert(false && "IO.print expects a String!"); },
        [](String &str) { std::cout << str << "\n"; },
        [](Int&) { assert(false && "IO.print expects a String!"); },
        [](RuntimeValue *v) { printStringValue(*v); }
    );
}

void handleDefaultIO(
    const EffectCtorRef &ecr,
    Context &context
) {
    if(ecr.effectCtorIndex == 0) {
        // IO.print(String)
        printStringValue(ecr.arguments[0].lower(context));
    }
}

}
