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
    std::vector<Value> &enclosingVariables
) {
    if(prog.config.debugLevel >= Config::LogLevel::LOUD)
        std::cout << "prove: " << pr << "\n";

    const auto &pd = prog.getPredicate(pr.index);
    for(const auto &impl : pd.implications) {
        if(prog.config.debugLevel >= Config::LogLevel::MAX)
            std::cout << "  try implication: " << impl << std::endl;
        std::vector<Value> localVariables(impl.variableCount);

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
    const EffectCtorRef &ecr,
    std::vector<Value> &enclosingVariables
) {
    if(prog.config.debugLevel >= Config::LogLevel::QUIET)
        std::cout << "handle effect: " << ecr << "\n";
    if(ecr.effectIndex == 0) {
        handleDefaultIO(ecr, enclosingVariables);
    }
    co_yield {};
}

Generator<Unit> witnesses(
    const Program &prog,
    const Conjunction conj,
    std::vector<Value> &variables
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
    std::vector<Value> &variables
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
        auto w = witnesses(prog, *pr, variables);
        while(w.next())
            co_yield {};
    } else if(expr.as_a<EffectCtorRef>().unwrapInto(ecr)) {
        auto w = witnesses(prog, *ecr, variables);
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
    std::vector<Value> &existentialVariables,
    std::vector<Value> &universalVariables
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
    std::vector<Value> &existentialVariables,
    std::vector<Value> &universalVariables
) {
    assert(vr.index != VariableRef::anonymousIndex);
    assert(vr.isExistential);

    if( matcher.index == VariableRef::anonymousIndex ||
        (vr.index == matcher.index &&
         vr.isExistential == matcher.isExistential))
        return vr.isTypeInhabited;

    if(vr.isDefinition) {
        if(matcher.isDefinition) {
            if(!vr.isTypeInhabited)
                return false;
            VariableRef u = matcher;
            u.isExistential = true;
            universalVariables[matcher.index] = Value(u);
            existentialVariables[vr.index] = Value(&universalVariables[matcher.index]);
            return true;
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

// TODO: passing cr by value does a lot of unnecessary copying in deep structures.
/// Replaces each remaining variable ref in the given constructor value with a pointer
/// to the universally quantified (local) variable it represents. The result of this
/// function should never be stored back into `universalVariables`, as these variables
/// will point to themselves.
static Value replaceUnboundVariableRefsWithPointers(
    ConstructorRef cr,
    std::vector<Value> &universalVariables
) {
    for(Value &arg : cr.arguments) {
        arg = arg.match<Value>(
        [&](ConstructorRef cr2) {
            return replaceUnboundVariableRefsWithPointers(cr2, universalVariables);
        },
        [](String str) { return Value(str); },
        [](Int i) { return Value(i); },
        [](Value *vp) { return Value(vp); },
        [&](VariableRef vr) {
            if(vr.index == VariableRef::anonymousIndex)
                return Value(vr);
            assert(!vr.isExistential);
            universalVariables[vr.index] = Value(VariableRef(vr.index, true, true, vr.isTypeInhabited));
            return Value(&universalVariables[vr.index]);
        });
    }
    return cr;
}

bool match(
    const VariableRef &vr,
    const ConstructorRef &cr,
    std::vector<Value> &existentialVariables,
    std::vector<Value> &universalVariables
) {
    if(vr.index == VariableRef::anonymousIndex) return true;
    if(vr.isExistential) {
        assert(vr.index < existentialVariables.size());
        if(vr.isDefinition) {
            if(!vr.isTypeInhabited)
                return false;
            existentialVariables[vr.index] = replaceUnboundVariableRefsWithPointers(cr, universalVariables);
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

template <typename T>
static bool match(
    const VariableRef &vr,
    const T &t,
    std::vector<Value> &existentialVariables,
    std::vector<Value> &universalVariables
) {
    if(vr.index == VariableRef::anonymousIndex) return true;
    if(vr.isExistential) {
        assert(vr.index < existentialVariables.size());
        if(vr.isDefinition) {
            existentialVariables[vr.index] = Value(t);
            return true;
        } else {
            return existentialVariables[vr.index] == Value(t);
        }
    } else {
        assert(vr.index < universalVariables.size());
        if(vr.isDefinition) {
            universalVariables[vr.index] = Value(t);
            return true;
        } else {
            auto v = universalVariables[vr.index];
            VariableRef tmp;
            if(v.as_a<VariableRef>().unwrapInto(tmp)) {
                if(tmp.isExistential) {
                    return match(existentialVariables[tmp.index], Value(t), existentialVariables, universalVariables);
                } else {
                    return match(universalVariables[tmp.index], Value(t), existentialVariables, universalVariables);
                }
            } else {
                return v == Value(t);
            }
        }
    }
}

template bool match(
    const VariableRef &vr,
    const String &str,
    std::vector<Value> &existentialVariables,
    std::vector<Value> &universalVariables
);

template bool match(
    const VariableRef &vr,
    const Int &i,
    std::vector<Value> &existentialVariables,
    std::vector<Value> &universalVariables
);

bool match(
    const ConstructorRef &cl,
    const ConstructorRef &cr,
    std::vector<Value> &existentialVariables,
    std::vector<Value> &universalVariables
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
    std::vector<Value> &existentialVariables,
    std::vector<Value> &universalVariables
) {
    return left.match<bool>(
    [&](ConstructorRef cl) {
        return right.match<bool>(
        [&](ConstructorRef cr) {
            return match(cl, cr, existentialVariables, universalVariables);
        },
        [](String) { return false; },
        [](Int) { return false; },
        [&](Value *v) {
            return match(left, *v, existentialVariables, universalVariables);
        },
        [&](VariableRef vr) {
            return match(vr, cl, existentialVariables, universalVariables);
        });
    },
    [&](String lstr) {
        return right.match<bool>(
        [](ConstructorRef) { return false; },
        [&](String rstr) { return lstr == rstr; },
        [&](Int) { return false; },
        [&](Value *v) {
            return match(left, *v, existentialVariables, universalVariables);
        },
        [&](VariableRef vr) {
            return match(vr, lstr, existentialVariables, universalVariables);
        });
    },
    [&](Int i) {
        return right.match<bool>(
        [](ConstructorRef) { return false; },
        [](String) { return false; },
        [&](Int j) { return i == j; },
        [&](Value *v) {
            return match(Value(i), *v, existentialVariables, universalVariables);
        },
        [&](VariableRef vr) {
            return match(vr, i, existentialVariables, universalVariables);
        });
    },
    [&](Value *v) {
        return match(*v, right, existentialVariables, universalVariables);
    },
    [&](VariableRef vl) {
        return right.match<bool>(
        [&](ConstructorRef cr) {
            return match(vl, cr, existentialVariables, universalVariables);
        },
        [&](String rstr) {
            return match(vl, rstr, existentialVariables, universalVariables);
        },
        [&](Int r) {
            return match(vl, r, existentialVariables, universalVariables);
        },
        [&](Value *v) {
            return match(left, *v, existentialVariables, universalVariables);
        },
        [&](VariableRef vr) {
            return match(vl, vr, existentialVariables, universalVariables);
        });
    });
}


static Value instantiate(Value v, const std::vector<Value> &variables);

static Value instantiate(
    VariableRef vr,
    const std::vector<Value> &variables
) {
    if( vr.index != VariableRef::anonymousIndex &&
        !vr.isExistential &&
        variables[vr.index] != Value()) {
            return interpreter::Value(variables[vr.index]);
    } else {
        return interpreter::Value(vr);
    }
}

static Value instantiate(
    ConstructorRef cr,
    const std::vector<Value> &variables
) {
    for(Value &arg : cr.arguments) {
        arg = instantiate(arg, variables);
    }
    return cr;
}

static Value instantiate(
    Value v,
    const std::vector<Value> &variables
) {
    return v.match<Value>(
    [&](ConstructorRef cr) { return instantiate(cr, variables); },
    [&](String) { return v; },
    [&](Int) { return v; },
    [&](Value *vp) {
        // vp should always point to a variable in an enclosed scope, and
        // therefore can never contain references to variables in the current
        // scope.
        return v;
    },
    [&](VariableRef vr) { return instantiate(vr, variables); }
    );
}

Expression instantiate(
    const Expression &expr,
    const std::vector<Value> &variables
) {
    return expr.match<Expression>(
    [](TruthValue tv) { return interpreter::Expression(tv); },
    [&](PredicateReference pr) {
        for(Value &arg : pr.arguments) {
            arg = instantiate(arg, variables);
        }
        return interpreter::Expression(pr);
    },
    [&](EffectCtorRef ecr) {
        for(Value &arg : ecr.arguments) {
            arg = instantiate(arg, variables);
        }
        return interpreter::Expression(ecr);
    },
    [&](Conjunction conj) {
        return interpreter::Expression(Conjunction(
            instantiate(conj.getLeft(), variables),
            instantiate(conj.getRight(), variables)
        ));
    });
}

} // namespace interpreter
