#include <iostream>

#include "Interpreter/Program.h"
#include "Interpreter/WitnessProducer.h"

namespace interpreter {

bool Program::prove(const Expression &expr) {
    // TODO: if `main` ever takes arguments, they need to be allocated here.
    std::vector<Value> mainArgs;

    if(witnesses(*this, expr, mainArgs).next()) {
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
    [&](Conjunction conj) -> std::ostream& { return out << conj; }
    );
}

std::ostream& operator<<(std::ostream &out, const ConstructorRef &ctor) {
    out << ctor.index << "(";
    for(const auto &arg : ctor.arguments) out << arg << ", ";
    return out << ")";
}

std::ostream& operator<<(std::ostream &out, const String &str) {
    return out << str.value;
}

std::ostream& operator<<(std::ostream &out, const VariableRef &vr) {
    if(vr.index == VariableRef::anonymousIndex)
        out << "var _";
    else
        out << "var " << vr.index;
    if(vr.isDefinition)
        out << " def";
    return out;
}

std::ostream& operator<<(std::ostream &out, const Value &val) {
    return val.match<std::ostream&>(
    [&](ConstructorRef cr) -> std::ostream& { return out << cr; },
    [&](String str) -> std::ostream& { return out << str; },
    [&](Value *v) -> std::ostream& { return out << "ptr " << *v; },
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

std::ostream& operator<<(std::ostream &out, const Program &prog) {
    out << "Program\n";
    for(const auto &pred : prog.predicates)
        out << "    " << pred << "\n";
    return out;
}

} // namespace interpreter
