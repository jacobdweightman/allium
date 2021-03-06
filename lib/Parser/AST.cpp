#include <algorithm>

#include "Parser/AST.h"
#include "Parser/ASTPrinter.h"

namespace parser {

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
    ASTPrinter(out).visit(tl);
    return out;
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

bool operator==(const Value &lhs, const Value &rhs) {
    return lhs.location == rhs.location && lhs.isDefinition == rhs.isDefinition &&
        lhs.name == rhs.name && lhs.arguments == rhs.arguments;
}

bool operator!=(const Value &lhs, const Value &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const Value &val) {
    ASTPrinter(out).visit(val);
    return out;
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

Optional<Type> AST::resolveTypeRef(const TypeRef &tr) const {
    const auto &x = std::find_if(
        types.begin(),
        types.end(),
        [&](const Type &type) { return type.declaration.name == tr.name; });
    
    if(x == types.end()) {
        return Optional<Type>();
    } else {
        return *x;
    }
}

Optional<Predicate> AST::resolvePredicateRef(const PredicateRef &pr) const {
    const auto &x = std::find_if(
        predicates.begin(),
        predicates.end(),
        [&](const Predicate &p) { return p.name.name == pr.name; });
    
    if(x == predicates.end()) {
        return Optional<Predicate>();
    } else {
        return *x;
    }
}

bool operator==(const EffectDecl &lhs, const EffectDecl &rhs) {
    return lhs.location == rhs.location && lhs.name == rhs.name;
}

bool operator!=(const EffectDecl &lhs, const EffectDecl &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const EffectDecl &decl) {
    return out << decl.name;
}

bool operator==(const EffectConstructor &lhs, const EffectConstructor &rhs) {
    return lhs.location == rhs.location && lhs.name == rhs.name &&
        lhs.parameters == rhs.parameters;
}

bool operator!=(const EffectConstructor &lhs, const EffectConstructor &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const EffectConstructor &ctor) {
    out << "ctor " << ctor.name << "(";
    for(const auto &param : ctor.parameters) {
        out << param << ", ";
    }
    return out << ")";
}

bool operator==(const Effect &lhs, const Effect &rhs) {
    return lhs.declaration == rhs.declaration &&
        lhs.constructors == rhs.constructors;
}

bool operator!=(const Effect &lhs, const Effect &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const Effect &effect) {
    ASTPrinter(out).visit(effect);
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

} // namespace parser
