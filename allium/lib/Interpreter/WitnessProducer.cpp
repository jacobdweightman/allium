#include <vector>

#include "Interpreter/BuiltinEffects.h"
#include "Interpreter/BuiltinPredicates.h"
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
    const BuiltinPredicateReference &bpr,
    Context &context
) {
    if(prog.config.debugLevel >= Config::LogLevel::LOUD)
        std::cout << "prove: " << bpr << "\n";

    size_t n = bpr.arguments.size();

    std::vector<RuntimeValue> args;
    for(size_t i=0; i<n; ++i) {
        args.push_back(bpr.arguments[i].lower(context));
    }

    return bpr.predicate(args);
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
    std::unique_ptr<BuiltinPredicateReference> bpr;
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
    } else if(expr.as_a<BuiltinPredicateReference>().unwrapInto(bpr)) {
        auto w = witnesses(prog, *bpr, context);
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
        assert(false && "unhandled expression type");
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
        auto leftVal = goalPred.arguments[i].lower(parentContext);
        auto rightVal = matcherPred.arguments[i].lower(localContext);
        if(!match(leftVal, rightVal))
            return false;
    }
    return true;
}

bool match(RuntimeValue *var1, RuntimeValue *var2) {
    if(isVarTypeUninhabited(var1) || isVarTypeUninhabited(var2))
        return false;

    if(!var1 || !var2)
        return true;

    RuntimeValue &val1 = var1->getValue();
    if(val1.isDefined()) {
        RuntimeValue &val2 = var2->getValue();
        if(val2.isDefined()) {
            return match(val1, val2);
        } else {
            val2 = RuntimeValue(&val1);
            return true;
        }
    } else {
        val1 = RuntimeValue(var2);
        return true;
    }
}

bool match(RuntimeValue *var, RuntimeCtorRef &ctor) {
    if(isVarTypeUninhabited(var))
        return false;

    if(isAnonymousVariable(var))
        return true;

    RuntimeValue &val = var->getValue();
    if(val.isDefined()) {
        RuntimeValue ctorVal(ctor);
        return match(val, ctorVal);
    } else {
        val = RuntimeValue(ctor);
        return true;
    }
}

bool match(RuntimeValue *var, const String &str) {
    if(isVarTypeUninhabited(var)) return false;
    if(isAnonymousVariable(var)) return true;
    RuntimeValue &val = var->getValue();
    if(val.isDefined()) {
        return val == RuntimeValue(str);
    } else {
        val = RuntimeValue(str);
        return true;
    }
}

bool match(RuntimeValue *var, const Int &i) {
    if(isVarTypeUninhabited(var)) return false;
    if(isAnonymousVariable(var)) return true;
    RuntimeValue &val = var->getValue();
    if(val.isDefined()) {
        return val == RuntimeValue(i);
    } else {
        val = RuntimeValue(i);
        return true;
    }
}

bool match(RuntimeCtorRef &ctor1, RuntimeCtorRef &ctor2) {
    if(ctor1.index != ctor2.index) return false;
    assert(ctor1.arguments.size() == ctor2.arguments.size());
    for(int i=0; i<ctor1.arguments.size(); ++i) {
        if(!match(ctor1.arguments[i], ctor2.arguments[i]))
            return false;
    }
    return true;
}

bool match(RuntimeValue &val1, RuntimeValue &val2) {
    return val1.match<bool>(
        [](std::monostate) { assert(false); return false; },
        [&](RuntimeCtorRef &lctor) {
            return val2.match<bool>(
                [](std::monostate) { assert(false); return false; },
                [&](RuntimeCtorRef &rctor) { return match(lctor, rctor); },
                [](String &rstr) { return false; },
                [](Int) { return false; },
                [&](RuntimeValue *rvar) { return match(rvar, lctor); }
            );
        },
        [&](String &lstr) {
            return val2.match<bool>(
                [](std::monostate) { assert(false); return false; },
                [](RuntimeCtorRef &) { return false; },
                [&](String &rstr) { return lstr == rstr; },
                [](Int) { return false; },
                [&](RuntimeValue *rvar) { return match(rvar, lstr); }
            );
        },
        [&](Int lint) {
            return val2.match<bool>(
                [](std::monostate) { assert(false); return false; },
                [](RuntimeCtorRef &) { return false; },
                [](String &rstr) { return false; },
                [&](Int rint) { return lint == rint; },
                [&](RuntimeValue *rvar) { return match(rvar, lint); }
            );
        },
        [&](RuntimeValue *lvar) {
            return val2.match<bool>(
                [](std::monostate) { assert(false); return false; },
                [&](RuntimeCtorRef &rctor) { return match(lvar, rctor); },
                [&](String &rstr) { return match(lvar, rstr); },
                [&](Int rint) { return match(lvar, rint); },
                [&](RuntimeValue *rvar) { return match(lvar, rvar); }
            );
        }
    );
}

} // namespace interpreter
