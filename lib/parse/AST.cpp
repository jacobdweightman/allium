#include "AST.h"
#include "ASTPrinter.h"

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

std::ostream& operator<<(std::ostream &out, const TruthLiteral &tl) {
    return out << "<TruthLiteral " << (tl.value ? "true" : "false") << ">";
}

std::ostream& operator<<(std::ostream &out, const PredicateDecl &pd) {
    ASTPrinter(out).visit(pd);
    return out;
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

std::ostream& operator<<(std::ostream &out, const Implication &impl) {
    return out << impl.lhs << " <- " << impl.rhs;
}

std::ostream& operator<<(std::ostream &out, const Predicate &pred) {
    out << "pred " << pred.name << " {\n";

    for(Implication impl : pred.implications) {
        out << "\t" << impl << "\n";
    }

    out << "}";
    return out;
}

std::ostream& operator<<(std::ostream &out, const TypeDecl &td) {
    out << td.name;
    return out;
}

std::ostream& operator<<(std::ostream &out, const TypeRef &typeRef) {
    out << typeRef.name;
    return out;
}

std::ostream& operator<<(std::ostream &out, const Constructor &ctor) {
    ASTPrinter(out).visit(ctor);
    return out;
}

std::ostream& operator<<(std::ostream &out, const ConstructorRef &cr) {
    ASTPrinter(out).visit(cr);
    return out;
}

std::ostream& operator<<(std::ostream &out, const Type &type) {
    out << "type " << type.declaration << " {\n";

    for(Constructor ctor : type.constructors) {
        out << ctor << "\n";
    }

    out << "}";
    return out;
}
