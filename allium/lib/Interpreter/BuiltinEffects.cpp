#include <vector>

#include "Interpreter/Program.h"
#include "Interpreter/WitnessProducer.h"

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

Generator<Unit> builtinHandlerIO(
    const Program &prog,
    const EffectCtorRef &ecr,
    Context &context,
    HandlerStack &handlers
) {
    switch(ecr.effectCtorIndex) {
    case 0:
        // IO.print(String)
        printStringValue(ecr.arguments[0].lower(context));
        break;
    default:
        assert(false && "unknown IO effect");
    }

    auto k = witnesses(prog, ecr.getContinuation(), context, handlers);
    while(k.next())
        co_yield {};
}

}
