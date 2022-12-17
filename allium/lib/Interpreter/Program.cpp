#include <iostream>
#include <sstream>

#include "Interpreter/BuiltinEffects.h"
#include "Interpreter/BuiltinPredicates.h"
#include "Interpreter/Program.h"
#include "Interpreter/WitnessProducer.h"
#include "Utils/VectorUtils.h"

namespace interpreter {

bool Program::prove(const Expression &expr) {
    // TODO: if `main` ever takes arguments, they need to be allocated here.
    Context mainContext;
    HandlerStack handlers;
    handlers.emplace_back(0, builtinHandlerIO);

    if(witnesses(*this, expr, mainContext, handlers).next()) {
        return true;
    } else {
        return false;
    }
}

bool operator==(const EffectImplHead &left, const EffectImplHead &right) {
    return left.effectIndex == right.effectIndex &&
        left.effectCtorIndex == right.effectCtorIndex &&
        left.arguments == right.arguments;
}

bool operator!=(const EffectImplHead &left, const EffectImplHead &right) {
    return !(left == right);
}

bool operator==(const Implication &left, const Implication &right) {
    return left.head == right.head && left.body == right.body &&
        left.variableCount == right.variableCount;
}

bool operator!=(const Implication &left, const Implication &right) {
    return !(left == right);
}

bool operator==(const EffectImplication &left, const EffectImplication &right) {
    return left.head == right.head && left.body == right.body &&
        left.variableCount == right.variableCount;
}

bool operator!=(const EffectImplication &left, const EffectImplication &right) {
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

std::string Program::asDebugString(const PredicateReference &pr) const {
    std::stringstream argstring;
    for(const auto &arg : pr.arguments) argstring << arg << ", ";

    return predicateNameTable[pr.index] + "(" + argstring.str() + ")";
}

std::ostream& operator<<(std::ostream &out, const Expression &expr) {
    return expr.match<std::ostream&>(
    [&](TruthValue tv) -> std::ostream& { return out << tv; },
    [&](PredicateReference pr) -> std::ostream& { return out << pr; },
    [&](BuiltinPredicateReference bpr) -> std::ostream& { return out << bpr; },
    [&](EffectCtorRef ecr) -> std::ostream& { return out << ecr; },
    [&](Conjunction conj) -> std::ostream& { return out << conj; }
    );
}

std::ostream& operator<<(std::ostream &out, const HandlerExpression &hExpr) {
    return hExpr.match<std::ostream&>(
    [&](TruthValue tv) -> std::ostream& { return out << tv; },
    [&](Continuation k) -> std::ostream& { return out << k; },
    [&](PredicateReference pr) -> std::ostream& { return out << pr; },
    [&](BuiltinPredicateReference bpr) -> std::ostream& { return out << bpr; },
    [&](EffectCtorRef ecr) -> std::ostream& { return out << ecr; },
    [&](HandlerConjunction hConj) -> std::ostream& { return out << hConj; }
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

RuntimeValue &RuntimeValue::getValue() {
    return match<RuntimeValue&>(
        [&](std::monostate) -> RuntimeValue& { return *this; },
        [&](RuntimeCtorRef&) -> RuntimeValue& { return *this; },
        [&](String&) -> RuntimeValue& { return *this; },
        [&](Int) -> RuntimeValue& { return *this; },
        [](RuntimeValue *v) -> RuntimeValue& { return v->getValue(); }
    );
}

std::ostream& operator<<(std::ostream &out, const RuntimeValue &v) {
    v.switchOver(
        [&](std::monostate) { out << "undefined"; },
        [&](RuntimeCtorRef &rcr) { out << rcr; },
        [&](String &str) { out << str; },
        [&](Int i) { out << i; },
        [&](RuntimeValue *rv) { out << *rv; }
    );
    return out;
}


RuntimeValue uninhabitedTypeVal;
RuntimeValue *uninhabitedTypeVar = &uninhabitedTypeVal;

RuntimeValue MatcherValue::lower(Context &context) const {
    return match<RuntimeValue>(
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
        [&](MatcherVariable &v) -> RuntimeValue {
            if(!v.isTypeInhabited) {
                return RuntimeValue(uninhabitedTypeVar);
            } else if(v.index == MatcherVariable::anonymousIndex) {
                return RuntimeValue(nullptr);
            } else {
                assert(v.index < context.size());
                return RuntimeValue(&context[v.index]);
            }
        }
    );
}

std::ostream& operator<<(std::ostream &out, const MatcherValue &val) {
    val.switchOver(
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

bool operator==(const Continuation &, const Continuation &) { return true; }

bool operator!=(const Continuation &, const Continuation &) { return false; }

std::ostream& operator<<(std::ostream &out, const Continuation &k) {
    return out << "continue";
}

std::ostream& operator<<(std::ostream &out, const PredicateReference &pr) {
    out << pr.index << "(";
    for(const auto &arg : pr.arguments) out << arg << ", ";
    return out << ")";
}

std::ostream& operator<<(std::ostream &out, const BuiltinPredicateReference &bpr) {
    out << getBuiltinPredicateName(bpr.predicate) << "(";
    for(const auto &arg : bpr.arguments) out << arg << ", ";
    return out << ")";
}

EffectCtorRef::EffectCtorRef(
    size_t effectIndex,
    size_t effectCtorIndex,
    std::vector<MatcherValue> arguments,
    Expression continuation
): effectIndex(effectIndex), effectCtorIndex(effectCtorIndex),
    arguments(arguments), _continuation(new auto(continuation)) {}

EffectCtorRef::EffectCtorRef(const EffectCtorRef &other):
    effectIndex(other.effectIndex),
    effectCtorIndex(other.effectCtorIndex),
    arguments(other.arguments),
    _continuation(new auto(*other._continuation)) {}

EffectCtorRef& EffectCtorRef::operator=(EffectCtorRef other) {
    using std::swap;
    effectIndex = other.effectIndex;
    effectCtorIndex = other.effectCtorIndex;
    std::swap(arguments, other.arguments);
    std::swap(_continuation, other._continuation);
    return *this;
}

Expression& EffectCtorRef::getContinuation() const { return *_continuation; }

std::ostream& operator<<(std::ostream &out, const EffectCtorRef &ecr) {
    return out << "do " << ecr.effectIndex << "." << ecr.effectCtorIndex <<
        " { " << ecr.getContinuation() << " }";
}

std::ostream& operator<<(std::ostream &out, const EffectImplHead &eih) {
    return out << "do " << eih.effectIndex << "." << eih.effectCtorIndex;
}

std::ostream& operator<<(std::ostream &out, const Conjunction &conj) {
    return out << "(" << conj.getLeft() << " and " << conj.getRight() << ")";
}

HandlerConjunction::HandlerConjunction(HandlerExpression left, HandlerExpression right):
    left(new auto(left)), right(new auto(right)) {}

HandlerConjunction::HandlerConjunction(const HandlerConjunction &other):
    left(new auto(*other.left)), right(new auto(*other.right)) {}

HandlerConjunction HandlerConjunction::operator=(HandlerConjunction other) {
    using std::swap;
    swap(left, other.left);
    swap(right, other.right);
    return *this;
}

bool operator==(const HandlerConjunction &left, const HandlerConjunction &right) {
    return left.getLeft() == right.getLeft() && left.getRight() == right.getRight();
}
bool operator!=(const HandlerConjunction &left, const HandlerConjunction &right) {
    return !(left == right);
}

std::ostream& operator<<(std::ostream &out, const HandlerConjunction &hConj) {
    return out << "(" << hConj.getLeft() << " and " << hConj.getRight() << ")";
}

std::ostream& operator<<(std::ostream &out, const Implication &impl) {
    return out << impl.head << " <- " << impl.body;
}

std::ostream& operator<<(std::ostream &out, const EffectImplication &eImpl) {
    return out << eImpl.head << " <- " << eImpl.body << ";";
}

std::ostream& operator<<(std::ostream &out, const UserHandler &h) {
    out << "handle " << h.effect << "{\n";
    for(const auto &hImpl : h.implications) out << "    " << hImpl << "\n";
    return out << "}";
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
