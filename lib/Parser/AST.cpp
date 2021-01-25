#include "Parser/AST.h"
#include "Parser/ASTPrinter.h"

std::ostream& operator<<(std::ostream &out, const Expression &e) {
    return e.match<std::ostream&>(
        [&](TruthLiteral tl) -> std::ostream& { out << tl; return out; },
        [&](PredicateRef p) -> std::ostream& { out << p; return out; },
        [&](Conjunction conj) -> std::ostream& {
            out << "(" << conj.getLeft() << " and " << conj.getRight() << ")";
            return out;
        }
    );
}

bool operator==(const TruthLiteral &lhs, const TruthLiteral &rhs) {
    return lhs.value == rhs.value && lhs.location == rhs.location;
}

bool operator!=(const TruthLiteral &lhs, const TruthLiteral &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const TruthLiteral &tl) {
    return out << "<TruthLiteral " << (tl.value ? "true" : "false") << ">";
}

bool operator==(const PredicateDecl &lhs, const PredicateDecl &rhs) {
    return lhs.location == rhs.location && lhs.name == rhs.name &&
        lhs.parameters == rhs.parameters;
}

bool operator!=(const PredicateDecl &lhs, const PredicateDecl &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const PredicateDecl &pd) {
    ASTPrinter(out).visit(pd);
    return out;
}

bool operator==(const PredicateRef &lhs, const PredicateRef &rhs) {
    return lhs.name == rhs.name && lhs.location == rhs.location &&
        lhs.arguments == rhs.arguments;
}

bool operator!=(const PredicateRef &lhs, const PredicateRef &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const PredicateRef &p) {
    ASTPrinter(out).visit(p);
    return out;
}

Conjunction::Conjunction(Expression left, Expression right):
    _left(new auto(left)), _right(new auto(right)) {}

Conjunction::Conjunction(const Conjunction &other):
    _left(new auto(*other._left)), _right(new auto(*other._right)) {}

std::ostream& operator<<(std::ostream &out, const Conjunction &conj) {
    return out << "Conjunction\n" <<
        conj.getLeft() << "\n" <<
        conj.getRight() << ">";
}

bool operator==(const Conjunction &lhs, const Conjunction &rhs) {
    // Note: (A and B) != (B and A)
    return lhs.getLeft() == rhs.getLeft() && lhs.getRight() == rhs.getRight();
}

bool operator!=(const Conjunction &lhs, const Conjunction &rhs) {
    return !(lhs == rhs);
}

bool operator==(const Implication &lhs, const Implication &rhs) {
    return lhs.lhs == rhs.lhs && lhs.rhs == rhs.rhs;
}

inline bool operator!=(const Implication &lhs, const Implication &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const Implication &impl) {
    return out << impl.lhs << " <- " << impl.rhs;
}

bool operator==(const Predicate &lhs, const Predicate &rhs) {
    return lhs.name == rhs.name && lhs.implications == rhs.implications;
}

bool operator!=(const Predicate &lhs, const Predicate &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const Predicate &pred) {
    out << "pred " << pred.name << " {\n";

    for(Implication impl : pred.implications) {
        out << "\t" << impl << "\n";
    }

    out << "}";
    return out;
}

bool operator==(const TypeDecl &lhs, const TypeDecl &rhs) {
    return lhs.name == rhs.name && lhs.location == rhs.location;
}

bool operator!=(const TypeDecl &lhs, const TypeDecl &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const TypeDecl &td) {
    out << td.name;
    return out;
}

bool operator==(const TypeRef &lhs, const TypeRef &rhs) {
    return lhs.name == rhs.name && lhs.location == rhs.location;
}

bool operator!=(const TypeRef &lhs, const TypeRef &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const TypeRef &typeRef) {
    out << typeRef.name;
    return out;
}

bool operator==(const Constructor &lhs, const Constructor &rhs) {
    return lhs.name == rhs.name && lhs.location == rhs.location;
}

bool operator!=(const Constructor &lhs, const Constructor &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const Constructor &ctor) {
    ASTPrinter(out).visit(ctor);
    return out;
}

bool operator==(const AnonymousVariable &lhs, const AnonymousVariable &rhs) {
    return lhs.location == rhs.location;
}

bool operator!=(const AnonymousVariable &lhs, const AnonymousVariable &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const AnonymousVariable &av) {
    ASTPrinter(out).visit(av);
    return out;
}

bool operator==(const Variable &lhs, const Variable &rhs) {
    return lhs.name == rhs.name && lhs.location == rhs.location;
}

bool operator!=(const Variable &lhs, const Variable &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const Variable &v) {
    ASTPrinter(out).visit(v);
    return out;
}

bool operator==(const ConstructorRef &lhs, const ConstructorRef &rhs) {
    return lhs.name == rhs.name && lhs.location == rhs.location &&
        lhs.arguments == rhs.arguments;
}

bool operator!=(const ConstructorRef &lhs, const ConstructorRef &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const ConstructorRef &cr) {
    ASTPrinter(out).visit(cr);
    return out;
}

std::ostream& operator<<(std::ostream &out, const Value &val) {
    return val.match<std::ostream&>(
    [&](AnonymousVariable av) -> std::ostream& { return out << av; },
    [&](Variable x) -> std::ostream& { return out << x; },
    [&](ConstructorRef cr) -> std::ostream& { return out << cr; }
    );
}

bool operator==(const Type &lhs, const Type &rhs) {
    return lhs.declaration == rhs.declaration &&
        lhs.constructors == rhs.constructors;
}

bool operator!=(const Type &lhs, const Type &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const Type &type) {
    out << "type " << type.declaration << " {\n";

    for(Constructor ctor : type.constructors) {
        out << ctor << "\n";
    }

    out << "}";
    return out;
}

bool operator==(const AST &lhs, const AST &rhs) {
    return lhs.types == rhs.types && lhs.predicates == rhs.predicates;
}

bool operator!=(const AST &lhs, const AST &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const AST &ast) {
    for(const auto &predicate : ast.predicates) {
        out << predicate << "\n";
    }

    for(const auto &type : ast.types) {
        out << type << "\n";
    }

    return out;
}
