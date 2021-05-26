#include "Interpreter/program.h"
#include "values/Generator.h"
#include "values/Unit.h"

namespace interpreter {

bool match(
    const PredicateReference &pr,
    const PredicateReference &matcher,
    std::vector<ConstructorRef> &existentialVariables,
    std::vector<ConstructorRef> &universalVariables
);

bool match(
    const VariableRef &vr,
    const VariableRef &matcher,
    std::vector<ConstructorRef> &existentialVariables,
    std::vector<ConstructorRef> &universalVariables
);

bool match(
    const VariableRef &vr,
    const ConstructorRef &cr,
    std::vector<ConstructorRef> &existentialVariables,
    std::vector<ConstructorRef> &universalVariables
);

bool match(
    const ConstructorRef &cl,
    const ConstructorRef &cr,
    std::vector<ConstructorRef> &existentialVariables,
    std::vector<ConstructorRef> &universalVariables
);

bool match(
    const Value &left,
    const Value &right,
    std::vector<ConstructorRef> &existentialVariables,
    std::vector<ConstructorRef> &universalVariables
);

Expression instantiate(
    const Expression &expr,
    const std::vector<ConstructorRef> &variables
);

/**
 * A generator which enumerates the witnesses of expr.
 * 
 * @param prog The enclosing program in which to resolve predicates.
 * @param expr The expression to be proven.
 * @param variables Enclosing scope in which to lookup variable values.
 */
Generator<Unit> witnesses(
    const Program &prog,
    const Expression expr,
    std::vector<ConstructorRef> &variables
);

Generator<Unit> witnesses(const TruthValue &tv);

Generator<Unit> witnesses(
    const Program &prog,
    const PredicateReference &pr,
    std::vector<ConstructorRef> &enclosingVariables
);

Generator<Unit> witnesses(
    const Program &prog,
    const Conjunction conj,
    std::vector<ConstructorRef> &variables
);

} // namespace interpreter
