#include <iostream>

#include "Interpreter/Program.h"
#include "Interpreter/WitnessProducer.h"
#include "Utils/VectorUtils.h"

namespace interpreter {

bool Program::prove(const Expression &expr) {
    // TODO: if `main` ever takes arguments, they need to be allocated here.
    Context mainContext;

    if(witnesses(*this, expr, mainContext).next()) {
        return true;
    } else {
        return false;
    }
}

bool operator==(const Implication &left, const Implication &right) {
    return left.head == right.head && left.body == right.body &&
        left.variableCount == right.variableCount;
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

std::ostream& operator<<(std::ostream &out, const Expression &expr) {
    return expr.match<std::ostream&>(
    [&](TruthValue tv) -> std::ostream& { return out << tv; },
    [&](PredicateReference pr) -> std::ostream& { return out << pr; },
    [&](EffectCtorRef ecr) -> std::ostream& { return out << ecr; },
    [&](Conjunction conj) -> std::ostream& { return out << conj; }
    );
}

std::ostream& operator<<(std::ostream &out, const RuntimeCtorRef &ctor) {
    out << ctor.index << "(";
    for(const auto &arg : ctor.arguments) out << arg << ", ";
    return out << ")";
}

std::ostream& operator<<(std::ostream &out, const MatcherCtorRef &ctor) {
    out << ctor.index << "(";
    for(const auto &arg : ctor.arguments) out << arg << ", ";
    return out << ")";
}

std::ostream& operator<<(std::ostream &out, const String &str) {
    return out << str.value;
}

std::ostream& operator<<(std::ostream &out, const Int &i) {
    return out << i.value;
}

std::ostream& operator<<(std::ostream &out, const MatcherVariable &vr) {
    if(vr.index == MatcherVariable::anonymousIndex)
        out << "var _";
    else
        out << "var " << vr.index;
    return out;
}

MatcherValue RuntimeValue::lift() const {
    return visit(
        [](std::monostate) { assert(false); return MatcherValue(); },
        [&](RuntimeCtorRef &cr) {
            return MatcherValue(
                MatcherCtorRef(
                    cr.index,
                    map<RuntimeValue, MatcherValue>(
                        cr.arguments,
                        [&](RuntimeValue v) { return v.lift(); }
                    )
                )
            );
        },
        [](String &str) { return MatcherValue(str); },
        [](Int i) { return MatcherValue(i); },
        [](RuntimeValue *v) { return v->lift(); }
    );
}

RuntimeValue &RuntimeValue::getValue() {
    return visit(overloaded {
        [&](std::monostate) -> RuntimeValue& { return *this; },
        [&](RuntimeCtorRef&) -> RuntimeValue& { return *this; },
        [&](String&) -> RuntimeValue& { return *this; },
        [&](Int) -> RuntimeValue& { return *this; },
        [](RuntimeValue *v) -> RuntimeValue& { return v->getValue(); }
    });
}

std::ostream& operator<<(std::ostream &out, const RuntimeValue &v) {
    v.visit(
        [&](std::monostate) { out << "undefined"; },
        [&](RuntimeCtorRef &rcr) { out << rcr; },
        [&](String &str) { out << str; },
        [&](Int i) { out << i; },
        [&](RuntimeValue *rv) { out << *rv; }
    );
    return out;
}

RuntimeValue MatcherValue::lower(Context &context) const {
    return visit(
        [](std::monostate) { assert(false); return RuntimeValue(); },
        [&](MatcherCtorRef &mCtor) {
            return RuntimeValue(
                RuntimeCtorRef(
                    mCtor.index,
                    map<MatcherValue, RuntimeValue>(
                        mCtor.arguments,
                        [&](MatcherValue v) { return v.lower(context); }
                    )
                )
            );
        },
        [](String &str) { return RuntimeValue(str); },
        [](Int i) { return RuntimeValue(i); },
        [&](MatcherVariable &v) { return RuntimeValue(&context[v.index]); }
    );
}

std::ostream& operator<<(std::ostream &out, const MatcherValue &val) {
    val.visit(
        [&](std::monostate) { assert(false); },
        [&](MatcherCtorRef &cr) { out << cr; },
        [&](String &str) { out << str; },
        [&](Int i) { out << i; },
        [&](MatcherVariable &mr) { out << mr; }
    );
    return out;
}

std::ostream& operator<<(std::ostream &out, const TruthValue &tv) {
    return out << (tv.value ? "true" : "false");
}

std::ostream& operator<<(std::ostream &out, const PredicateReference &pr) {
    out << pr.index << "(";
    for(const auto &arg : pr.arguments) out << arg << ", ";
    return out << ")";
}

std::ostream& operator<<(std::ostream &out, const EffectCtorRef &ecr) {
    return out << "do " << ecr.effectIndex << "." << ecr.effectCtorIndex;
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

std::ostream& operator<<(std::ostream &out, const Program &prog) {
    out << "Program\n";
    for(const auto &pred : prog.predicates)
        out << "    " << pred << "\n";
    return out;
}

} // namespace interpreter
