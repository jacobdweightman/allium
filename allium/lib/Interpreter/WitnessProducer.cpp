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
        if(!match(goalPred.arguments[i], matcherPred.arguments[i], parentContext, localContext))
            return false;
    }
    return true;
}

bool match(
    const MatcherVariable &goalVar,
    const MatcherVariable &matcherVar,
    Context &parentContext,
    Context &localContext
) {
    assert(goalVar.index != MatcherVariable::anonymousIndex);

    if(matcherVar.index == MatcherVariable::anonymousIndex)
        return goalVar.isTypeInhabited;

    RuntimeValue &goalVarVal = parentContext[goalVar.index].getValue();
    if(goalVarVal.isDefined()) {
        RuntimeValue &matcherVarVal = localContext[matcherVar.index].getValue();
        if(matcherVarVal.isDefined()) {
            return match(
                goalVarVal.lift(),
                matcherVarVal.lift(),
                parentContext,
                localContext);
        } else {
            matcherVarVal = RuntimeValue(&goalVarVal);
            return true;
        }
    } else {
        if(!goalVar.isTypeInhabited)
            return false;
        // VariableRef u = matcher;
        // u.isExistential = true;
        // localContext[matcher.index] = VariableValue(u);
        goalVarVal = RuntimeValue(&localContext[matcherVar.index]);
        return true;
    }
}

bool match(
    const MatcherVariable &goalVar,
    const MatcherCtorRef &matcherCtor,
    Context &parentContext,
    Context &localContext
) {
    if(goalVar.index == MatcherVariable::anonymousIndex) return true;
    assert(goalVar.index < parentContext.size());

    RuntimeValue &varValue = parentContext[goalVar.index].getValue();
    if(varValue.isDefined()) {
        return match(
            varValue.lift(),
            MatcherValue(matcherCtor),
            parentContext,
            localContext);
    } else {
        if(!goalVar.isTypeInhabited)
            return false;
        varValue = MatcherValue(matcherCtor).lower(localContext);
        return true;
    }
}

bool match(
    const MatcherCtorRef &goalCtor,
    const MatcherVariable &matcherVar,
    Context &parentContext,
    Context &localContext
) {
    if(matcherVar.index == MatcherVariable::anonymousIndex) return true;
    assert(matcherVar.index < localContext.size());

    RuntimeValue &varValue = localContext[matcherVar.index].getValue();
    if(varValue.isDefined()) {
        return match(
            MatcherValue(goalCtor),
            varValue.lift(),
            parentContext,
            localContext);
    } else {
        varValue = MatcherValue(goalCtor).lower(parentContext);
        return true;
    }
}

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
        return variableVal.getValue() == RuntimeValue(t);
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
    const MatcherCtorRef &goalCtor,
    const MatcherCtorRef &matcherCtor,
    Context &parentContext,
    Context &localContext
) {
    if(goalCtor.index != matcherCtor.index) return false;
    assert(goalCtor.arguments.size() == matcherCtor.arguments.size());
    for(int i=0; i<goalCtor.arguments.size(); ++i) {
        if(!match(goalCtor.arguments[i], matcherCtor.arguments[i], parentContext, localContext))
            return false;
    }
    return true;
}

bool match(
    const MatcherValue &goalVal,
    const MatcherValue &matcherVal,
    Context &parentContext,
    Context &localContext
) {

    return goalVal.match<bool>(
        [](std::monostate) { assert(false); return false; },
        [&](MatcherCtorRef &lctor) {
            return matcherVal.match<bool>(
                [](std::monostate) { assert(false); return false; },
                [&](MatcherCtorRef &rctor) { return match(lctor, rctor, parentContext, localContext); },
                [](String &rstr) { return false; },
                [](Int) { return false; },
                [&](MatcherVariable &rmv) { return match(lctor, rmv, parentContext, localContext); }
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
        [&](MatcherVariable &lmv) {
            return matcherVal.match<bool>(
                [](std::monostate) { assert(false); return false; },
                [&](MatcherCtorRef &rctor) { return match(lmv, rctor, parentContext, localContext); },
                [&](String &rstr) { return match(lmv, rstr, parentContext); },
                [&](Int rint) { return match(lmv, rint, parentContext); },
                [&](MatcherVariable &rmv) { return match(lmv, rmv, parentContext, localContext); }
            );
        }
    );
}

} // namespace interpreter
