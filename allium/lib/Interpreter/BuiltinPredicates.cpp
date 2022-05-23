#include "Interpreter/BuiltinPredicates.h"
#include "Interpreter/Program.h"
#include "Utils/Generator.h"
#include "Utils/Unit.h"

namespace interpreter {

std::map<std::string, BuiltinPredicate> builtinPredicateDefinitionTable;
std::map<BuiltinPredicate, const char *> builtinPredicateNameTable;

BuiltinPredicate getBuiltinPredicateByName(const std::string &name) {
    return builtinPredicateDefinitionTable[name];
}

std::string getBuiltinPredicateName(BuiltinPredicate bp) {
    return builtinPredicateNameTable[bp];
}

// The variable register_name is created in order to use its static initializer
// to register variables. If the C++ compiler is smart, it will see that these
// variables are never used and delete their storage while keeping the effect of
// the initializer.
//
// Note that the registration precedes the definition to ensure that all builtin
// predicates are also declared in the header file.
#define BUILTIN(name, args) \
    static int register_ ## name = ([]() { \
        builtinPredicateDefinitionTable.insert({ #name, &name }); \
        builtinPredicateNameTable.insert({ &name, #name }); \
        return 0; \
    })(); \
    Generator<Unit> name(std::vector<RuntimeValue> args)

BUILTIN(concat, args) {
    RuntimeValue &a = args[0].getValue();
    RuntimeValue &b = args[1].getValue();
    RuntimeValue &c = args[2].getValue();

    String aStr, bStr;

    // TODO: generalize to allow non-ground a and b
    if(a.as_a<String>().unwrapGuard(aStr)) {
        assert(false && "concat's first argument must be ground");
    }
    if(b.as_a<String>().unwrapGuard(bStr)) {
        assert(false && "concat's second argument must be ground");
    }
    bool hasSingleWitness = c.match<bool>(
        [&](std::monostate&) {
            c = RuntimeValue(String(aStr.value + bStr.value));
            return true;
        },
        [](RuntimeCtorRef&) {
            assert(false && "concat expects a String!");
            return false;
        },
        [&](String &cStr) {
            return aStr.value + bStr.value == cStr.value;
        },
        [](Int&) { assert(false && "concat expects a String!"); return false; },
        [](RuntimeValue *v) { assert(false && "unreachable"); return false; }
    );

    if(hasSingleWitness) {
        co_yield {};
    }
}

#undef BUILTIN

} // namespace interpreter
