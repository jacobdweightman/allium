#include <algorithm>

#include "Parser/AST.h"
#include "Parser/ASTPrinter.h"
#include "Parser/Builtins.h"

namespace parser {

std::ostream& operator<<(std::ostream &out, const Expression &e) {
    return e.match<std::ostream&>(
        [&](TruthLiteral tl) -> std::ostream& { out << tl; return out; },
        [&](Continuation k) -> std::ostream& { out << k; return out; },
        [&](PredicateRef p) -> std::ostream& { out << p; return out; },
        [&](EffectCtorRef ecr) -> std::ostream& { out << ecr; return out; },
        [&](Conjunction conj) -> std::ostream& {
            out << "(" << conj.getLeft() << " and " << conj.getRight() << ")";
            return out;
        }
    );
}

std::ostream& operator<<(std::ostream &out, const Value &val) {
    return val.match<std::ostream&>(
    [&](NamedValue nv) -> std::ostream& { out << nv; return out; },
    [&](StringLiteral str) -> std::ostream& { out << str; return out; },
    [&](IntegerLiteral i) -> std::ostream& { out << i; return out; }
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

bool operator==(const Continuation &lhs, const Continuation &rhs) {
    return lhs.location == rhs.location;
}

bool operator!=(const Continuation &lhs, const Continuation &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const Continuation &k) {
    ASTPrinter(out).visit(k);
    return out;
}

bool operator==(const PredicateDecl &lhs, const PredicateDecl &rhs) {
    return lhs.location == rhs.location && lhs.name == rhs.name &&
        lhs.parameters == rhs.parameters && lhs.effects == rhs.effects;
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

bool operator==(const EffectImplHead &lhs, const EffectImplHead &rhs) {
    return lhs.location == rhs.location && lhs.name == rhs.name &&
        lhs.arguments == rhs.arguments;
}

bool operator!=(const EffectImplHead &lhs, const EffectImplHead &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const EffectImplHead &eih) {
    ASTPrinter(out).visit(eih);
    return out;
}

EffectCtorRef::EffectCtorRef(
    std::string name,
    std::vector<Value> arguments,
    const Expression &continuation,
    SourceLocation location
): name(name), arguments(arguments), _continuation(new auto(continuation)),
    location(location) {}

EffectCtorRef::EffectCtorRef(const EffectCtorRef &other):
    name(other.name), arguments(other.arguments), location(other.location),
    _continuation(new auto(*other._continuation)) {}

EffectCtorRef& EffectCtorRef::operator=(EffectCtorRef other) {
    using std::swap;
    swap(name, other.name);
    swap(arguments, other.arguments);
    swap(_continuation, other._continuation);
    swap(location, other.location);
    return *this;
}

Expression& EffectCtorRef::getContinuation() const { return *_continuation; }

bool operator==(const EffectCtorRef &lhs, const EffectCtorRef &rhs) {
    return lhs.location == rhs.location && lhs.name == rhs.name &&
        lhs.arguments == rhs.arguments &&
        lhs.getContinuation() == rhs.getContinuation();
}

bool operator!=(const EffectCtorRef &lhs, const EffectCtorRef &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const EffectCtorRef &p) {
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

bool operator==(const CtorParameter &lhs, const CtorParameter &rhs) {
    return lhs.name == rhs.name && lhs.location == rhs.location;
}

bool operator!=(const CtorParameter &lhs, const CtorParameter &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const CtorParameter &cp) {
    out << cp.name;
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

bool operator==(const NamedValue &lhs, const NamedValue &rhs) {
    return lhs.location == rhs.location && lhs.isDefinition == rhs.isDefinition &&
        lhs.name == rhs.name && lhs.arguments == rhs.arguments;
}

bool operator!=(const NamedValue &lhs, const NamedValue &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const NamedValue &val) {
    ASTPrinter(out).visit(val);
    return out;
}

bool operator==(const StringLiteral &lhs, const StringLiteral &rhs) {
    return lhs.location == rhs.location && lhs.text == rhs.text;
}

bool operator!=(const StringLiteral &lhs, const StringLiteral &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const StringLiteral &str) {
    ASTPrinter(out).visit(str);
    return out;
}

bool operator==(const IntegerLiteral &lhs, const IntegerLiteral &rhs) {
    return lhs.value == rhs.value && lhs.location == rhs.location;
}

bool operator!=(const IntegerLiteral &lhs, const IntegerLiteral &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const IntegerLiteral &i) {
    ASTPrinter(out).visit(i);
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

Optional<Type> AST::resolveTypeRef(const Name<Type> &tr) const {
    // TODO: this works well for literal types, but won't work well
    // for builtin types with constructors.
    if(nameIsBuiltinType(tr))
        return Type(TypeDecl(tr.string(), SourceLocation()), {});

    const auto &x = std::find_if(
        types.begin(),
        types.end(),
        [&](const Type &type) { return type.declaration.name == tr; });

    if(x == types.end()) {
        return Optional<Type>();
    } else {
        return *x;
    }
}

Optional<const Effect*> AST::resolveEffectRef(const EffectRef &er) const {
    auto x = std::find_if(
        effects.begin(),
        effects.end(),
        [&](const Effect &e) { return e.declaration.name == er.name; });

    if(x == effects.end()) {
        x = std::find_if(
            builtinEffects.begin(),
            builtinEffects.end(),
            [&](const Effect &e) { return e.declaration.name == er.name; });
        
        if(x == builtinEffects.end()) {
            return Optional<const Effect*>();
        } else {
            return &*x;
        }
    } else {
        return &*x;
    }
}

Optional<std::pair<const Effect*, const EffectConstructor*>>
AST::resolveEffectCtorRef(const EffectCtorRef &ecr) const {
    for(const auto &e : builtinEffects) {
        const auto eCtor = std::find_if(
            e.constructors.begin(),
            e.constructors.end(),
            [&](const EffectConstructor &eCtor) { return eCtor.name == ecr.name; });
        
        if(eCtor != e.constructors.end()) {
            return std::make_pair(&e, &*eCtor);
        }
    }

    for(const auto &e : effects) {
        const auto eCtor = std::find_if(
            e.constructors.begin(),
            e.constructors.end(),
            [&](const EffectConstructor &eCtor) { return eCtor.name == ecr.name; });
        
        if(eCtor != e.constructors.end()) {
            return std::make_pair(&e, &*eCtor);
        }
    }
    return Optional<std::pair<const Effect*, const EffectConstructor*>>();
}

Optional<PredicateDecl> AST::resolvePredicateRef(const PredicateRef &pr) const {
    const auto &x = std::find_if(
        predicates.begin(),
        predicates.end(),
        [&](const Predicate &p) { return p.name.name == pr.name; });
    
    if(x != predicates.end()) {
        return x->name;
    }

    const auto &y = std::find_if(
        builtinPredicates.begin(),
        builtinPredicates.end(),
        [&](const PredicateDecl &pDecl) { return pDecl.name == pr.name; });

    if(y == builtinPredicates.end()) {
        return Optional<PredicateDecl>();
    } else {
        return *y;
    }
}

bool operator==(const EffectRef &lhs, const EffectRef &rhs) {
    return lhs.location == rhs.location && lhs.name == rhs.name;
}

bool operator!=(const EffectRef &lhs, const EffectRef &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const EffectRef &e) {
    ASTPrinter(out).visit(e);
    return out;
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

bool operator==(const Parameter &lhs, const Parameter &rhs) {
    return lhs.name == rhs.name && lhs.isInputOnly == rhs.isInputOnly &&
        lhs.location == rhs.location;
}

bool operator!=(const Parameter &lhs, const Parameter &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const Parameter &cp) {
    out << (cp.isInputOnly ? "in " : "") << cp.name;
    return out;
}

bool operator==(const EffectConstructor &lhs, const EffectConstructor &rhs) {
    return lhs.location == rhs.location && lhs.name == rhs.name &&
        lhs.parameters == rhs.parameters;
}

bool operator!=(const EffectConstructor &lhs, const EffectConstructor &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const EffectConstructor &eCtor) {
    ASTPrinter(out).visit(eCtor);
    return out;
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

bool operator==(const Handler &lhs, const Handler &rhs) {
    return lhs.effect == rhs.effect &&
        lhs.implications == rhs.implications;
}

bool operator!=(const Handler &lhs, const Handler &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const Handler &h) {
    ASTPrinter(out).visit(h);
    return out;
}

bool operator==(const EffectImplication &lhs, const EffectImplication &rhs) {
    return lhs.head == rhs.head &&
        lhs.body == rhs.body;
}

bool operator!=(const EffectImplication &lhs, const EffectImplication &rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const EffectImplication &ei) {
    ASTPrinter(out).visit(ei);
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
