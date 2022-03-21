#include <vector>

#include "Interpreter/BuiltinEffects.h"
#include "Interpreter/WitnessProducer.h"

namespace interpreter {

Generator<Unit> witnesses(const TruthValue &tv) {
    if(tv.value)
        co_yield {};
}

Generator<Unit> witnesses(
    const Program &prog,
    const PredicateReference &pr,
    Context &context
) {
    if(prog.config.debugLevel >= Config::LogLevel::LOUD)
        std::cout << "prove: " << prog.asDebugString(pr) << "\n";

    // Save the original context so that mutations from pattern matching don't
    // persist beyond backtracking.
    Context originalContextCopy = context;

    const auto &pd = prog.getPredicate(pr.index);
    for(const auto &impl : pd.implications) {
        if(prog.config.debugLevel >= Config::LogLevel::MAX)
            std::cout << "  try implication: " << impl << std::endl;
        Context localContext(impl.variableCount);

        if(match(pr, impl.head, context, localContext)) {
            auto w = witnesses(prog, impl.body, localContext);
            while(w.next())
                co_yield {};
        }

        // Restore the context to its original state between each implication
        // to undo mutations from matching the previous implication's head.
        context = originalContextCopy;
    }
}

Generator<Unit> witnesses(
    const Program &prog,
    const EffectCtorRef &ecr,
    Context &context
) {
    if(prog.config.debugLevel >= Config::LogLevel::QUIET)
        std::cout << "handle effect: " << ecr << "\n";
    if(ecr.effectIndex == 0) {
        handleDefaultIO(ecr, context);
    }
    co_yield {};
}

Generator<Unit> witnesses(
    const Program &prog,
    const Conjunction conj,
    Context &context
) {
    auto leftW = witnesses(prog, conj.getLeft(), context);
    while(leftW.next()) {
        auto rightW = witnesses(prog, conj.getRight(), context);

        while(rightW.next()) {
            co_yield {};
        }
    }
}

Generator<Unit> witnesses(
    const Program &prog,
    const Expression expr,
    Context &context
) {
    // TODO: generators and functions don't compose, and so we can't use
    // Expression::switchOver here
    std::unique_ptr<TruthValue> tv;
    std::unique_ptr<PredicateReference> pr;
    std::unique_ptr<EffectCtorRef> ecr;
    std::unique_ptr<Conjunction> conj;
    if(expr.as_a<TruthValue>().unwrapInto(tv)) {
        auto w = witnesses(*tv);
        while(w.next())
            co_yield {};
    } else if(expr.as_a<PredicateReference>().unwrapInto(pr)) {
        auto w = witnesses(prog, *pr, context);
        while(w.next())
            co_yield {};
    } else if(expr.as_a<EffectCtorRef>().unwrapInto(ecr)) {
        auto w = witnesses(prog, *ecr, context);
        while(w.next())
            co_yield {};
    } else if(expr.as_a<Conjunction>().unwrapInto(conj)) {
        auto w = witnesses(prog, *conj, context);
        while(w.next())
            co_yield {};
    } else {
        // Unhandled expression type!
        std::cout << expr << std::endl;
        abort();
    }
}

bool match(
    const PredicateReference &goalPred,
    const PredicateReference &matcherPred,
    Context &parentContext,
    Context &localContext
) {
    if(goalPred.index != matcherPred.index)
        return false;

    for(int i=0; i<goalPred.arguments.size(); ++i) {
        auto argVal = goalPred.arguments[i].lower(parentContext);
        if(!match(argVal, matcherPred.arguments[i], localContext))
            return false;
    }
    return true;
}

bool match(
    RuntimeValue *goalVar,
    const MatcherVariable &matcherVar,
    Context &localContext
) {
    assert(goalVar && "goalVar must not be an anonymous variable.");

    if(matcherVar.index == MatcherVariable::anonymousIndex)
        return matcherVar.isTypeInhabited;

    RuntimeValue &goalVarVal = goalVar->getValue();
    if(goalVarVal.isDefined()) {
        RuntimeValue &matcherVarVal = localContext[matcherVar.index].getValue();
        if(matcherVarVal.isDefined()) {
            return match(
                goalVarVal,
                matcherVarVal.lift(),
                localContext);
        } else {
            matcherVarVal = RuntimeValue(&goalVarVal);
            return true;
        }
    } else {
        if(!matcherVar.isTypeInhabited)
            return false;
        goalVarVal = RuntimeValue(&localContext[matcherVar.index]);
        return true;
    }
}

bool match(
    RuntimeValue *goalVar,
    const MatcherCtorRef &matcherCtor,
    Context &localContext
) {
    // TODO: pattern matching for anonymous existential variables.
    RuntimeValue &varValue = goalVar->getValue();
    if(varValue.isDefined()) {
        return match(
            varValue,
            MatcherValue(matcherCtor),
            localContext);
    } else {
        // If there is a valid constructor to match the variable against, then
        // then the variable's type must be inhabited.
        varValue = MatcherValue(matcherCtor).lower(localContext);
        return true;
    }
}

bool match(
    RuntimeCtorRef &goalCtor,
    const MatcherVariable &matcherVar,
    Context &localContext
) {
    assert(matcherVar.isTypeInhabited);
    if(matcherVar.index == MatcherVariable::anonymousIndex) return true;
    assert(matcherVar.index < localContext.size());

    RuntimeValue &varValue = localContext[matcherVar.index].getValue();
    if(varValue.isDefined()) {
        RuntimeValue goalValue(goalCtor);
        return match(
            goalValue,
            varValue.lift(),
            localContext);
    } else {
        varValue = RuntimeValue(goalCtor);
        return true;
    }
}

template <typename T>
static bool match(
    RuntimeValue *var,
    const T &t
) {
    assert(var && "var must not be null!");
    RuntimeValue &val = var->getValue();
    if(val.isDefined()) {
        return val == RuntimeValue(t);
    } else {
        val = RuntimeValue(t);
        return true;
    }
}

template bool match(
    RuntimeValue *var,
    const String &str
);

template bool match(
    RuntimeValue *var,
    const Int &i
);

template <typename T>
static bool match(
    const MatcherVariable &vr,
    const T &t,
    Context &context
) {
    if(vr.index == MatcherVariable::anonymousIndex) return true;
    assert(vr.index < context.size());
    auto &variableVal = context[vr.index];
    if(variableVal.isDefined()) {
        return variableVal == RuntimeValue(t);
    } else {
        variableVal = RuntimeValue(t);
        return true;
    }
}

template bool match(
    const MatcherVariable &vr,
    const String &str,
    Context &context
);

template bool match(
    const MatcherVariable &vr,
    const Int &i,
    Context &context
);

bool match(
    RuntimeCtorRef &goalCtor,
    const MatcherCtorRef &matcherCtor,
    Context &localContext
) {
    if(goalCtor.index != matcherCtor.index) return false;
    assert(goalCtor.arguments.size() == matcherCtor.arguments.size());
    for(int i=0; i<goalCtor.arguments.size(); ++i) {
        if(!match(goalCtor.arguments[i], matcherCtor.arguments[i], localContext))
            return false;
    }
    return true;
}

bool match(
    RuntimeValue &goalVal,
    const MatcherValue &matcherVal,
    Context &localContext
) {

    return goalVal.match<bool>(
        [](std::monostate) { assert(false); return false; },
        [&](RuntimeCtorRef &lctor) {
            return matcherVal.match<bool>(
                [](std::monostate) { assert(false); return false; },
                [&](MatcherCtorRef &rctor) { return match(lctor, rctor, localContext); },
                [](String &rstr) { return false; },
                [](Int) { return false; },
                [&](MatcherVariable &rmv) { return match(lctor, rmv, localContext); }
            );
        },
        [&](String &lstr) {
            return matcherVal.match<bool>(
                [](std::monostate) { assert(false); return false; },
                [](MatcherCtorRef) { return false; },
                [&](String &rstr) { return lstr == rstr; },
                [](Int) { return false; },
                [&](MatcherVariable &rmv) { return match(rmv, lstr, localContext); }
            );
        },
        [&](Int lint) {
            return matcherVal.match<bool>(
                [](std::monostate) { assert(false); return false; },
                [](MatcherCtorRef) { return false; },
                [](String &rstr) { return false; },
                [&](Int rint) { return lint == rint; },
                [&](MatcherVariable &rmv) { return match(rmv, lint, localContext); }
            );
        },
        [&](RuntimeValue *lvar) {
            return matcherVal.match<bool>(
                [](std::monostate) { assert(false); return false; },
                [&](MatcherCtorRef &rctor) { return match(lvar, rctor, localContext); },
                [&](String &rstr) { return match(lvar, rstr); },
                [&](Int rint) { return match(lvar, rint); },
                [&](MatcherVariable &rmv) { return match(lvar, rmv, localContext); }
            );
        }
    );
}

} // namespace interpreter
