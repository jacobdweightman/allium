#include "Interpreter/WitnessProducer.h"

#include <vector>

namespace interpreter {

Generator<Unit> witnesses(const TruthValue &tv) {
    if(tv.value)
        co_yield {};
}

Generator<Unit> witnesses(
    const Program &prog,
    const PredicateReference &pr,
    std::vector<ConstructorRef> &enclosingVariables
) {
    std::cout << "prove: " << pr << "\n";
    const auto &pd = prog.getPredicate(pr.index);
    for(const auto &impl : pd.implications) {
        std::vector<ConstructorRef> localVariables(
            impl.headVariableCount + impl.bodyVariableCount
        );

        if(match(pr, impl.head, enclosingVariables, localVariables)) {
            Expression body = instantiate(impl.body, localVariables);

            auto w = witnesses(prog, body, localVariables);
            std::unique_ptr<std::vector<ConstructorRef> > bodyVariables;
            while(w.next())
                co_yield {};
        }
        
    }
}

Generator<Unit> witnesses(
    const Program &prog,
    const Conjunction conj,
    std::vector<ConstructorRef> &variables
) {
    auto leftW = witnesses(prog, conj.getLeft(), variables);
    while(leftW.next()) {
        auto rightW = witnesses(prog, conj.getRight(), variables);

        while(rightW.next()) {
            co_yield {};
        }
    }
}

Generator<Unit> witnesses(
    const Program &prog,
    const Expression expr,
    std::vector<ConstructorRef> &variables
) {
    // TODO: generators and functions don't compose, and so we can't use
    // Expression::switchOver here
    std::unique_ptr<TruthValue> tv;
    std::unique_ptr<PredicateReference> pr;
    std::unique_ptr<Conjunction> conj;
    if(expr.as_a<TruthValue>().unwrapInto(tv)) {
        auto w = witnesses(*tv);
        while(w.next())
            co_yield {};
    } else if(expr.as_a<PredicateReference>().unwrapInto(pr)) {
        auto w = witnesses(prog, *pr, variables);
        while(w.next())
            co_yield {};
    } else if(expr.as_a<Conjunction>().unwrapInto(conj)) {
        auto w = witnesses(prog, *conj, variables);
        while(w.next())
            co_yield {};
    } else {
        // Unhandled expression type!
        std::cout << expr << std::endl;
        abort();
    }
}

bool match(
    const PredicateReference &pr,
    const PredicateReference &matcher,
    std::vector<ConstructorRef> &existentialVariables,
    std::vector<ConstructorRef> &universalVariables
) {
    if(pr.index != matcher.index)
        return false;

    for(int i=0; i<pr.arguments.size(); ++i) {
        if(!match(pr.arguments[i], matcher.arguments[i], existentialVariables, universalVariables))
            return false;
    }
    return true;
}

bool match(
    const VariableRef &vr,
    const VariableRef &matcher,
    std::vector<ConstructorRef> &existentialVariables,
    std::vector<ConstructorRef> &universalVariables
) {
    assert(vr.isExistential);

    if( vr.index == matcher.index ||
        vr.index == VariableRef::anonymousIndex ||
        matcher.index == VariableRef::anonymousIndex) return true;

    if(vr.isDefinition) {
        if(matcher.isDefinition) {
            assert(false && "attempted to define two un-bound variables as equal");
            return false;
        } else {
            existentialVariables[vr.index] = universalVariables[matcher.index];
            return true;
        }
    } else {
        if(matcher.isDefinition) {
            universalVariables[matcher.index] = existentialVariables[vr.index];
            return true;
        } else {
            return match(
                existentialVariables[vr.index],
                universalVariables[matcher.index],
                existentialVariables,
                universalVariables);
        }
    }
}

bool match(
    const VariableRef &vr,
    const ConstructorRef &cr,
    std::vector<ConstructorRef> &existentialVariables,
    std::vector<ConstructorRef> &universalVariables
) {
    if(vr.index == VariableRef::anonymousIndex) return true;
    if(vr.isExistential) {
        assert(vr.index < existentialVariables.size());
        if(vr.isDefinition) {
            existentialVariables[vr.index] = cr;
            return true;
        } else {
            return match(
                existentialVariables[vr.index],
                cr,
                existentialVariables,
                universalVariables);
        }
    } else {
        assert(vr.index < universalVariables.size());
        if(vr.isDefinition) {
            universalVariables[vr.index] = cr;
            return true;
        } else {
            return match(
                universalVariables[vr.index],
                cr,
                existentialVariables,
                universalVariables);
        }
    }
}

bool match(
    const ConstructorRef &cl,
    const ConstructorRef &cr,
    std::vector<ConstructorRef> &existentialVariables,
    std::vector<ConstructorRef> &universalVariables
) {
    if(cl.index != cr.index) return false;
    assert(cl.arguments.size() == cr.arguments.size());
    for(int i=0; i<cl.arguments.size(); ++i) {
        if(!match(cl.arguments[i], cr.arguments[i], existentialVariables, universalVariables))
            return false;
    }
    return true;
}

bool match(
    const Value &left,
    const Value &right,
    std::vector<ConstructorRef> &existentialVariables,
    std::vector<ConstructorRef> &universalVariables
) {
    return left.match<bool>(
    [&](ConstructorRef cl) {
        return right.match<bool>(
        [&](ConstructorRef cr) {
            return match(cl, cr, existentialVariables, universalVariables);
        },
        [&](VariableRef vr) {
            return match(vr, cl, existentialVariables, universalVariables);
        });
    },
    [&](VariableRef vl) {
        return right.match<bool>(
        [&](ConstructorRef cr) {
            return match(vl, cr, existentialVariables, universalVariables);
        },
        [&](VariableRef vr) {
            return match(vl, vr, existentialVariables, universalVariables);
        });
    });
}

static ConstructorRef instantiate(
    ConstructorRef cr,
    const std::vector<ConstructorRef> &variables
) {
    for(Value &arg : cr.arguments) {
        VariableRef vr;
        if( arg.as_a<VariableRef>().unwrapInto(vr) &&
            vr.index != VariableRef::anonymousIndex &&
            variables[vr.index] != ConstructorRef()) {
                arg = variables[vr.index];
        }
    }
    return cr;
}

Expression instantiate(
    const Expression &expr,
    const std::vector<ConstructorRef> &variables
) {
    return expr.match<Expression>(
    [](TruthValue tv) { return interpreter::Expression(tv); },
    [&](PredicateReference pr) {
        for(Value &arg : pr.arguments) {
            arg = arg.match<Value>(
            [&](ConstructorRef cr) { return instantiate(cr, variables); },
            [&](VariableRef vr) -> Value {
                if( vr.index != VariableRef::anonymousIndex &&
                    !vr.isExistential &&
                    (assert(vr.index < variables.size()),
                     variables[vr.index] != ConstructorRef())) {
                        return interpreter::Value(variables[vr.index]);
                } else {
                    return interpreter::Value(vr);
                }
            });
        }
        return interpreter::Expression(pr);
    },
    [&](Conjunction conj) {
        return interpreter::Expression(Conjunction(
            instantiate(conj.getLeft(), variables),
            instantiate(conj.getRight(), variables)
        ));
    });
}

} // namespace interpreter
