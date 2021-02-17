#include "Interpreter/WitnessProducer.h"

namespace interpreter {

std::unique_ptr<WitnessProducer> createWitnessProducer(
    const interpreter::Program &program,
    interpreter::Expression expr
) {
    return expr.match<std::unique_ptr<WitnessProducer> >(
    [](interpreter::TruthValue tv) {
        return std::make_unique<TruthValueWitnessProducer>(TruthValueWitnessProducer(tv));
    },
    [&](interpreter::PredicateReference pr) {
        return std::make_unique<PredicateRefWitnessProducer>(PredicateRefWitnessProducer(program, pr));
    },
    [&](interpreter::Conjunction conj) {
        return std::make_unique<ConjunctionWitnessProducer>(ConjunctionWitnessProducer(program, conj));
    });
}

WitnessProducer::~WitnessProducer() {}

bool TruthValueWitnessProducer::nextWitness() {
    bool tmpNext = nextValue;
    nextValue = false;
    return tmpNext;
}

PredicateRefWitnessProducer::~PredicateRefWitnessProducer() {}

bool PredicateRefWitnessProducer::nextWitness() {
    while(currentImpl != predicate->implications.end()) {
        if(currentWitnessProducer->nextWitness()) {
            return true;
        } else {
            ++currentImpl;
            setUpNextMatchingImplication();
        }
    }
    return false;
}

bool PredicateRefWitnessProducer::match(const PredicateReference &other) {
    if(pr.index != other.index)
        return false;

    for(int i=0; i<pr.arguments.size(); ++i) {
        if(!match(pr.arguments[i], other.arguments[i]))
            return false;
    }
    return true;
}

bool PredicateRefWitnessProducer::match(const VariableRef &vl, const VariableRef &vr) {
    if( vl.index == vr.index ||
        vl.index == VariableRef::anonymousIndex ||
        vr.index == VariableRef::anonymousIndex) return true;

    if(vl.isDefinition) {
        if(vr.isDefinition) {
            assert(false && "attempted to define two un-bound variables as equal");
            return false;
        } else {
            universalVariables[vl.index] = universalVariables[vr.index];
            return true;
        }
    } else {
        if(vr.isDefinition) {
            universalVariables[vr.index] = universalVariables[vl.index];
            return true;
        } else {
            return match(universalVariables[vl.index], universalVariables[vr.index]);
        }
    }
}

bool PredicateRefWitnessProducer::match(const VariableRef &vr, const ConstructorRef &cr) {
    if(vr.index == VariableRef::anonymousIndex) return true;
    if(vr.isDefinition) {
        assert(vr.index < universalVariables.size());
        universalVariables.at(vr.index) = cr;
        return true;
    } else {
        assert(vr.index < universalVariables.size());
        return match(universalVariables.at(vr.index), cr);
    }
}

bool PredicateRefWitnessProducer::match(const ConstructorRef &cl, const ConstructorRef &cr) {
    if(cl.index != cr.index) return false;
    assert(cl.arguments.size() == cr.arguments.size());
    for(int i=0; i<cl.arguments.size(); ++i) {
        if(!match(cl.arguments[i], cr.arguments[i]))
            return false;
    }
    return true;
}

bool PredicateRefWitnessProducer::match(const Value &left, const Value &right) {
    return left.match<bool>(
    [&](ConstructorRef cl) {
        return right.match<bool>(
        [&](ConstructorRef cr) { return match(cl, cr); },
        [&](VariableRef vr) { return match(vr, cl); }
        );
    },
    [&](VariableRef vl) {
        return right.match<bool>(
        [&](ConstructorRef cr) { return match(vl, cr); },
        [&](VariableRef vr) { return match(vl, vr); }
        );
    });
}

bool ConjunctionWitnessProducer::nextWitness() {
    // TODO: clean up this logic. With a resumable function, it would look like this:
    // while(leftWitnessProducer->nextWitness()) {
    //     while(rightWitnessProducer->nextWitness()) {
    //         yield true;
    //     }
    // }
    // yield false;

    if(!foundLeftWitness) return false;

    while(!rightWitnessProducer->nextWitness()) {
        if((foundLeftWitness = leftWitnessProducer->nextWitness())) {
        } else {
            return false;
        }
    }
    return true;
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

static Expression instantiate_(
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
            instantiate_(conj.getLeft(), variables),
            instantiate_(conj.getRight(), variables)
        ));
    });
}

Expression WitnessProducer::instantiate(const Expression &expr) const {
    return instantiate_(expr, universalVariables);
}

} // namespace interpreter
