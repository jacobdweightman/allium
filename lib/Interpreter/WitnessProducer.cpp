#include "Interpreter/WitnessProducer.h"

#include <vector>

namespace interpreter {

Generator<std::vector<ConstructorRef> > witnesses(const TruthValue &tv) {
    if(tv.value)
        co_yield {};
}
Generator<std::vector<ConstructorRef> > witnesses(const Program &prog, const PredicateReference &pr) {
    std::cout << "prove: " << pr << "\n";
    const auto &pd = prog.getPredicate(pr.index);
    for(const auto &impl : pd.implications) {
        std::vector<ConstructorRef> variables(
            impl.headVariableCount + impl.bodyVariableCount
        );

        if(match(pr, impl.head, variables)) {
            Expression body = instantiate(impl.body, variables); // TODO: does this handle body variables appropriately?
            auto w = witnesses(prog, body);
            std::unique_ptr<std::vector<ConstructorRef> > bodyVariables;
            while(w.next().unwrapInto(bodyVariables))
                // TODO: bound head variables?
                co_yield *bodyVariables;
        }
        
    }
}
Generator<std::vector<ConstructorRef> > witnesses(const Program &prog, const Conjunction conj) {
    auto leftW = witnesses(prog, conj.getLeft());
    std::vector<ConstructorRef> leftWitness;
    while(leftW.next().unwrapInto(leftWitness)) {
        auto leftEnd = leftWitness.end();
        auto rightW = witnesses(prog, conj.getRight());
        std::vector<ConstructorRef> rightWitness;

        while(rightW.next().unwrapInto(rightWitness)) {
            // yield leftWitness concatenated with rightWitness
            leftWitness.insert(leftWitness.end(), rightWitness.begin(), rightWitness.end());
            co_yield leftWitness;
            leftWitness.erase(leftEnd, leftWitness.end());
        }
    }
}

Generator<std::vector<ConstructorRef> > witnesses(const Program &prog, const Expression expr) {
    // TODO: generators and functions don't compose, and so we can't use
    // Expression::switchOver here
    std::unique_ptr<TruthValue> tv;
    std::unique_ptr<PredicateReference> pr;
    std::unique_ptr<Conjunction> conj;
    std::vector<ConstructorRef> witness;
    if(expr.as_a<TruthValue>().unwrapInto(tv)) {
        auto w = witnesses(*tv);
        while(w.next().unwrapInto(witness))
            co_yield witness;
    } else if(expr.as_a<PredicateReference>().unwrapInto(pr)) {
        auto w = witnesses(prog, *pr);
        while(w.next().unwrapInto(witness))
            co_yield witness;
    } else if(expr.as_a<Conjunction>().unwrapInto(conj)) {
        auto w = witnesses(prog, *conj);
        while(w.next().unwrapInto(witness))
            co_yield witness;
    } else {
        // Unhandled expression type!
        std::cout << expr << std::endl;
        abort();
    }
}

bool match(
    const PredicateReference &pr,
    const PredicateReference &matcher,
    std::vector<ConstructorRef> &variables
) {
    if(pr.index != matcher.index)
        return false;

    for(int i=0; i<pr.arguments.size(); ++i) {
        if(!match(pr.arguments[i], matcher.arguments[i], variables))
            return false;
    }
    return true;
}

bool match(
    const VariableRef &vr,
    const VariableRef &matcher,
    std::vector<ConstructorRef> &variables
) {
    if( vr.index == matcher.index ||
        vr.index == VariableRef::anonymousIndex ||
        matcher.index == VariableRef::anonymousIndex) return true;

    if(vr.isDefinition) {
        if(matcher.isDefinition) {
            assert(false && "attempted to define two un-bound variables as equal");
            return false;
        } else {
            variables[vr.index] = variables[matcher.index];
            return true;
        }
    } else {
        if(matcher.isDefinition) {
            variables[matcher.index] = variables[vr.index];
            return true;
        } else {
            return match(variables[vr.index], variables[matcher.index], variables);
        }
    }
}

bool match(
    const VariableRef &vr,
    const ConstructorRef &cr,
    std::vector<ConstructorRef> &variables
) {
    if(vr.index == VariableRef::anonymousIndex) return true;
    assert(vr.index < variables.size());
    if(vr.isDefinition) {
        variables[vr.index] = cr;
        return true;
    } else {
        return match(variables.at(vr.index), cr, variables);
    }
}

bool match(
    const ConstructorRef &cl,
    const ConstructorRef &cr,
    std::vector<ConstructorRef> &variables
) {
    if(cl.index != cr.index) return false;
    assert(cl.arguments.size() == cr.arguments.size());
    for(int i=0; i<cl.arguments.size(); ++i) {
        if(!match(cl.arguments[i], cr.arguments[i], variables))
            return false;
    }
    return true;
}

bool match(
    const Value &left,
    const Value &right,
    std::vector<ConstructorRef> &variables
) {
    return left.match<bool>(
    [&](ConstructorRef cl) {
        return right.match<bool>(
        [&](ConstructorRef cr) { return match(cl, cr, variables); },
        [&](VariableRef vr) { return match(vr, cl, variables); }
        );
    },
    [&](VariableRef vl) {
        return right.match<bool>(
        [&](ConstructorRef cr) { return match(vl, cr, variables); },
        [&](VariableRef vr) { return match(vl, vr, variables); }
        );
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
