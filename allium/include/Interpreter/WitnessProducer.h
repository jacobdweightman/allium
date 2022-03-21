#ifndef INTERPRETER_WITNESS_PRODUCER_H
#define INTERPRETER_WITNESS_PRODUCER_H

#include "Interpreter/Program.h"
#include "Utils/Generator.h"
#include "Utils/Unit.h"

namespace interpreter {

bool match(
    const PredicateReference &goalPred,
    const PredicateReference &matcherPred,
    Context &parentContext,
    Context &localContext
);

bool match(
    RuntimeValue *goalVar,
    const MatcherVariable &matcherVar,
    Context &localContext
);

bool match(
    RuntimeValue *goalVar,
    const MatcherCtorRef &matcherCtor,
    Context &localContext
);

bool match(
    RuntimeCtorRef &goalCtor,
    const MatcherVariable &matcherVar,
    Context &localContext
);

bool match(
    RuntimeCtorRef &goalCtor,
    const MatcherCtorRef &matcherCtor,
    Context &localContext
);

bool match(
    RuntimeValue &goalVal,
    const MatcherValue &matcherVal,
    Context &localContext
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
    Context &context
);

Generator<Unit> witnesses(const TruthValue &tv);

Generator<Unit> witnesses(
    const Program &prog,
    const PredicateReference &pr,
    Context &context
);

Generator<Unit> witnesses(
    const Program &prog,
    const Conjunction conj,
    Context &context
);

} // namespace interpreter

#endif // INTERPRETER_WITNESS_PRODUCER_H
