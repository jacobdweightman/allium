#include "SemAna/TypedAST.h"

namespace TypedAST {

bool operator==(const TypeDecl &left, const TypeDecl &right) {
    return left.name == right.name;
}

bool operator!=(const TypeDecl &left, const TypeDecl &right) {
    return !(left == right);
}

bool operator==(const Constructor &left, const Constructor &right) {
    return left.name == right.name &&
        left.parameters == right.parameters;
}

bool operator!=(const Constructor &left, const Constructor &right) {
    return !(left == right);
}

Conjunction::Conjunction(Expression left, Expression right):
    _left(new auto(left)), _right(new auto(right)) {}

Conjunction::Conjunction(const Conjunction &other):
    _left(new auto(*other._left)), _right(new auto(*other._right)) {}

Expression &Conjunction::getLeft() const { return *_left; }

Expression &Conjunction::getRight() const { return *_right; }

bool operator==(const ConstructorRef &left, const ConstructorRef &right) {
    return left.name == right.name && left.arguments == right.arguments;
}

bool operator!=(const ConstructorRef &left, const ConstructorRef &right) {
    return !(left == right);
}

bool operator==(const Type &left, const Type &right) {
    return left.declaration == right.declaration &&
        left.constructors == right.constructors;
}

bool operator!=(const Type &left, const Type &right) {
    return !(left == right);
}

bool operator==(Variable left, Variable right) {
    return left.isDefinition == right.isDefinition && left.name == right.name &&
        left.type == right.type;
}

bool operator!=(Variable left, Variable right) {
    return !(left == right);
}

std::ostream& operator<<(std::ostream &out, const Value &val) {
    return val.match<std::ostream&>(
    [&](AnonymousVariable av) -> std::ostream& { return out << "_"; },
    [&](Variable var) -> std::ostream& {return out << var.name; },
    [&](ConstructorRef cr) -> std::ostream& {
        out << cr.name << "(";
        for(const auto &x : cr.arguments) out << x << ", ";
        return out << ")";
    });
}

std::ostream& operator<<(std::ostream &out, const PredicateRef &pr) {
    out << pr.name << "(";
    for(const auto &x : pr.arguments) out << x << ", ";
    return out << ")";
}

}
