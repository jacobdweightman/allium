#include <iostream>
#include "Interpreter/program.h"

bool interpreter::Program::prove(const Expression &expr) {
    return expr.match<bool>(
        [](TruthValue tv) -> bool { std::cout << "prove TV: " << tv.value << "\n"; return tv.value; },
        [&](PredicateReference pr) -> bool {
            std::cout << "prove PR: " << pr.index << "\n";
            Predicate p = predicates[pr.index];
            for(Implication implication : p.implications) {
                std::cout << "next implication\n";
                if(implication.matches(implication.head, pr) && prove(implication.instantiateBody()))
                    return true;
            }
            return false;
        },
        [&](Conjunction conj) -> bool {
            std::cout << "prove conjunction\n";
            return prove(conj.getLeft()) && prove(conj.getRight());
        }
    );
}
 
namespace interpreter {

bool operator==(const Implication &left, const Implication &right) {
    return left.head == right.head && left.body == right.body;
}

bool operator!=(const Implication &left, const Implication &right) {
    return !(left == right);
}

bool operator==(const Predicate &left, const Predicate &right) {
    return left.implications == right.implications;
}

bool operator!=(const Predicate &left, const Predicate &right) {
    return !(left == right);
}

bool operator==(const Program &left, const Program &right) {
    return left.predicates == right.predicates &&
        left.entryPoint == right.entryPoint;
}

bool operator!=(const Program &left, const Program &right) {
    return !(left == right);
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

static Expression instantiate(
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
                    variables[vr.index] != ConstructorRef()) {
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

Expression Implication::instantiateBody() const {
    return instantiate(body, variables);
}

bool Implication::matches(const VariableRef &vr, const ConstructorRef &cr) const {
    if(vr.index == VariableRef::anonymousIndex) return true;
    if(vr.isDefinition) {
        assert(vr.index < variables.size());
        variables[vr.index] = cr;
        return true;
    } else {
        assert(vr.index < variables.size());
        return matches(variables[vr.index], cr);
    }
}

bool Implication::matches(const VariableRef &vl, const VariableRef &vr) const {
    if( vl.index == vr.index ||
        vl.index == VariableRef::anonymousIndex ||
        vr.index == VariableRef::anonymousIndex) return true;

    if(vl.isDefinition) {
        if(vr.isDefinition) {
            assert(false && "attempted to define two un-bound variables as equal");
            return false;
        } else {
            variables[vl.index] = variables[vr.index];
            return true;
        }
    } else {
        if(vr.isDefinition) {
            variables[vr.index] = variables[vl.index];
            return true;
        } else {
            return matches(variables[vl.index], variables[vr.index]);
        }
    }
}

bool Implication::matches(const ConstructorRef &cl, const ConstructorRef &cr) const {
    if(cl.index != cr.index) return false;
    for(int i=0; i<cl.arguments.size(); ++i) {
        if(!matches(cl.arguments[i], cr.arguments[i]))
            return false;
    }
    return true;
}

bool Implication::matches(const Value &left, const Value &right) const {
    return left.match<bool>(
    [&](ConstructorRef cl) {
        return right.match<bool>(
        [&](ConstructorRef cr) { return matches(cl, cr); },
        [&](VariableRef vr) { return matches(vr, cl); }
        );
    },
    [&](VariableRef vl) {
        return right.match<bool>(
        [&](ConstructorRef cr) { return matches(vl, cr); },
        [&](VariableRef vr) { return matches(vl, vr); }
        );
    });
}

bool Implication::matches(const PredicateReference &left, const PredicateReference &right) const {
    if(left.index != right.index) return false;

    for(int i=0; i<left.arguments.size(); i++) {
        if(!matches(left.arguments[i], right.arguments[i]))
            return false;
    }
    return true;
}

std::ostream& operator<<(std::ostream &out, const Expression &expr) {
    return expr.match<std::ostream&>(
    [&](TruthValue tv) -> std::ostream& { return out << tv; },
    [&](PredicateReference pr) -> std::ostream& { return out << pr; },
    [&](Conjunction conj) -> std::ostream& { return out << conj; }
    );
}

std::ostream& operator<<(std::ostream &out, const ConstructorRef &ctor) {
    out << ctor.index << "(";
    for(const auto &arg : ctor.arguments) out << arg << ", ";
    return out << ")";
}

std::ostream& operator<<(std::ostream &out, const VariableRef &vr) {
    if(vr.index == VariableRef::anonymousIndex)
        return out << "var _";
    else
        return out << "var " << vr.index;
}

std::ostream& operator<<(std::ostream &out, const Value &val) {
    return val.match<std::ostream&>(
    [&](ConstructorRef cr) -> std::ostream& { return out << cr; },
    [&](VariableRef vr) -> std::ostream& { return out << vr; });
}

std::ostream& operator<<(std::ostream &out, const TruthValue &tv) {
    return out << (tv.value ? "true" : "false");
}

std::ostream& operator<<(std::ostream &out, const PredicateReference &pr) {
    out << pr.index << "(";
    for(const auto &arg : pr.arguments) out << arg << ", ";
    return out << ")";
}

std::ostream& operator<<(std::ostream &out, const Conjunction &conj) {
    return out << "(" << conj.getLeft() << " and " << conj.getRight() << ")";
}

std::ostream& operator<<(std::ostream &out, const Implication &impl) {
    return out << impl.head << " <- " << impl.body;
}

std::ostream& operator<<(std::ostream &out, const Predicate &p) {
    out << "pred {\n";
    for(const auto &impl : p.implications) out << "    " << impl << "\n";
    return out << "}"; 
}

} // namespace interpreter
