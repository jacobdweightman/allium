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
    Context &localContext);

bool match(
    const EffectCtorRef &goalEffect,
    const EffectImplHead &matcherEffect,
    Context &parentContext,
    Context &localContext);

bool match(RuntimeValue *var1, RuntimeValue *var2);
bool match(RuntimeValue *var, RuntimeCtorRef &ctor);
bool match(RuntimeCtorRef &ctor1, RuntimeCtorRef &ctor2);
bool match(RuntimeValue *var, const String &str);
bool match(RuntimeValue *var, const Int &i);
bool match(RuntimeValue &val1, RuntimeValue &val2);

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
    Context &context,
    HandlerStack &handlers);

Generator<Unit> witnesses(const TruthValue &tv);

Generator<Unit> witnesses(
    const Program &prog,
    const PredicateReference &pr,
    Context &context,
    HandlerStack &handlers);

template <int N>
Generator<Unit> witnesses(
    const Program &prog,
    BuiltinPredicateReference &bpr,
    Context &context);

Generator<Unit> witnesses(
    const Program &prog,
    const Conjunction conj,
    Context &context,
    HandlerStack &handlers);

Generator<Unit> witnesses(
    const Program &prog,
    const HandlerExpression hExpr,
    const Expression &continuation,
    Context &context,
    HandlerStack &handlers);

Generator<Unit> witnesses(
    const Program &prog,
    const HandlerConjunction hConj,
    const Expression &continuation,
    Context &context,
    HandlerStack &handlers);

} // namespace interpreter

#endif // INTERPRETER_WITNESS_PRODUCER_H
