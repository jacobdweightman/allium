#include "SemAna/Builtins.h"
#include "SemAna/TypedAST.h"

namespace TypedAST {

bool operator==(const TypeDecl &left, const TypeDecl &right) {
    return left.name == right.name;
}

bool operator!=(const TypeDecl &left, const TypeDecl &right) {
    return !(left == right);
}

bool operator==(const CtorParameter &left, const CtorParameter &right) {
    return left.type == right.type;
}

bool operator!=(const CtorParameter &left, const CtorParameter &right) {
    return !(left == right);
}

bool operator==(const Constructor &left, const Constructor &right) {
    return left.name == right.name &&
        left.parameters == right.parameters;
}

bool operator!=(const Constructor &left, const Constructor &right) {
    return !(left == right);
}

bool operator==(const EffectDecl &left, const EffectDecl &right) {
    return left.name == right.name;
}

bool operator!=(const EffectDecl &left, const EffectDecl &right) {
    return !(left == right);
}

bool operator==(const Parameter &left, const Parameter &right) {
    return left.isInputOnly == right.isInputOnly && left.type == right.type;
}

bool operator!=(const Parameter &left, const Parameter &right) {
    return !(left == right);
}

bool operator==(const EffectCtor &left, const EffectCtor &right) {
    return left.name == right.name && left.parameters == right.parameters;
}

bool operator!=(const EffectCtor &left, const EffectCtor &right) {
    return !(left == right);
}

bool operator==(const Effect &left, const Effect &right) {
    return left.declaration == right.declaration &&
        left.constructors == right.constructors;
}

bool operator!=(const Effect &left, const Effect &right) {
    return !(left == right);
}

bool operator==(const EffectImplication &left, const EffectImplication &right) {
    assert(false && "effect implications not implemented yet!");
}

bool operator!=(const EffectImplication &left, const EffectImplication &right) {
    return !(left == right);
}


bool operator==(const Handler &left, const Handler &right) {
    return left.implications == right.implications;
}

bool operator!=(const Handler &left, const Handler &right) {
    return !(left == right);
}

PredicateRef::PredicateRef(std::string name, std::vector<Value> arguments):
    name(name), arguments(arguments) {}

Conjunction::Conjunction(Expression left, Expression right):
    _left(new auto(left)), _right(new auto(right)) {}

Conjunction::Conjunction(const Conjunction &other):
    _left(new auto(*other._left)), _right(new auto(*other._right)) {}

Conjunction Conjunction::operator=(Conjunction other) {
    using std::swap;
    swap(_left, other._right);
    swap(_right, other._right);
    return *this;
}

Expression &Conjunction::getLeft() const { return *_left; }

Expression &Conjunction::getRight() const { return *_right; }

ConstructorRef::ConstructorRef(std::string name, std::vector<Value> arguments):
    name(name), arguments(arguments) {}

ConstructorRef::ConstructorRef(const ConstructorRef &other):
    name(other.name), arguments(other.arguments) {}

bool operator==(const ConstructorRef &left, const ConstructorRef &right) {
    return left.name == right.name && left.arguments == right.arguments;
}

bool operator!=(const ConstructorRef &left, const ConstructorRef &right) {
    return !(left == right);
}

size_t getConstructorIndex(const Type &type, const ConstructorRef &cr) {
    auto ctor = std::find_if(
        type.constructors.begin(),
        type.constructors.end(),
        [&](const Constructor &ctor) { return ctor.name == cr.name; });

    assert(ctor != type.constructors.end());
    return ctor - type.constructors.begin();
}

bool operator==(const Type &left, const Type &right) {
    return left.declaration == right.declaration &&
        left.constructors == right.constructors;
}

bool operator!=(const Type &left, const Type &right) {
    return !(left == right);
}

bool operator==(AnonymousVariable left, AnonymousVariable right) {
    return left.type == right.type;
}

bool operator!=(AnonymousVariable left, AnonymousVariable right) {
    return !(left == right);
}

bool operator==(Variable left, Variable right) {
    return left.isDefinition == right.isDefinition && left.name == right.name &&
        left.type == right.type;
}

bool operator!=(Variable left, Variable right) {
    return !(left == right);
}

bool operator==(const StringLiteral &left, const StringLiteral &right) {
    return left.value == right.value;
}

bool operator!=(const StringLiteral &left, const StringLiteral &right) {
    return !(left == right);
}

bool operator==(const IntegerLiteral &left, const IntegerLiteral &right) {
    return left.value == right.value;
}

bool operator!=(const IntegerLiteral &left, const IntegerLiteral &right) {
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
    },
    [&](StringLiteral str) -> std::ostream& {
        return out << "\"" << str.value << "\"";
    },
    [&](IntegerLiteral i) -> std::ostream& {
        return out << i.value;
    });
}

std::ostream& operator<<(std::ostream &out, const Expression &expr) {
    expr.switchOver(
    [&](TruthLiteral tl) { out << tl; },
    [&](const PredicateRef &pr) { out << pr; },
    [&](const EffectCtorRef &ecr) { out << ecr; },
    [&](const Conjunction &conj) { out << conj.getLeft() << ", " << conj.getRight(); }
    );
    return out;
}

std::ostream& operator<<(std::ostream &out, const TruthLiteral &tl) {
    return out << (tl.value ? "true" : "false");
}

std::ostream& operator<<(std::ostream &out, const PredicateRef &pr) {
    out << pr.name << "(";
    for(const auto &x : pr.arguments) out << x << ", ";
    return out << ")";
}

std::ostream& operator<<(std::ostream &out, const EffectCtorRef &ecr) {
    return out << "do " << ecr.effectName << "." << ecr.ctorName;
}

std::ostream& operator<<(std::ostream &out, const Implication &impl) {
    return out << impl.head << " <- " << impl.body;
}

const Type &AST::resolveTypeRef(const Name<Type> &tr) const {
    auto x = std::find_if(
        types.begin(),
        types.end(),
        [&](const Type &type) { return type.declaration.name == tr; });

    if(x == types.end()) {
        x = std::find_if(
            builtinTypes.begin(),
            builtinTypes.end(),
            [&](const Type &type) { return type.declaration.name == tr; });
        assert(x != builtinTypes.end());
    }

    return *x;
}

const Constructor &AST::resolveConstructorRef(const Name<Type> &tr, const ConstructorRef &cr) const {
    const Type &type = resolveTypeRef(tr);

    const auto ctor = std::find_if(
        type.constructors.begin(),
        type.constructors.end(),
        [&](const Constructor &ctor) { return ctor.name == cr.name; });

    assert(ctor != type.constructors.end());
    return *ctor;
}

const Effect &AST::resolveEffectRef(const Name<Effect> &er) const {
    auto effect = std::find_if(
        effects.begin(),
        effects.end(),
        [&](const Effect &e) { return e.declaration.name == er; });
    
    if(effect == effects.end()) {
        effect = std::find_if(
            builtinEffects.begin(),
            builtinEffects.end(),
            [&](const Effect &e) { return e.declaration.name == er; });
        assert(effect != builtinEffects.end());
    }

    return *effect;
}

const EffectCtor &AST::resolveEffectCtorRef(const EffectCtorRef &ecr) const {
    const Effect &effect = resolveEffectRef(ecr.effectName);

    const auto eCtor = std::find_if(
        effect.constructors.begin(),
        effect.constructors.end(),
        [&](const EffectCtor &eCtor) { return eCtor.name == ecr.ctorName; });
    
    assert(eCtor != effect.constructors.end());
    return *eCtor;
}

const Predicate AST::resolvePredicateRef(const PredicateRef &pr) const {
    const auto up = std::find_if(
        predicates.begin(),
        predicates.end(),
        [&](const UserPredicate &up) { return up.declaration.name == pr.name; });

    if(up == predicates.end()) {
        const auto bp = std::find_if(
            builtinPredicates.begin(),
            builtinPredicates.end(),
            [&](const BuiltinPredicate &bp) { return bp.declaration.name == pr.name; });
        assert(bp != builtinPredicates.end());
        return Predicate(&*bp);
    }

    assert(up != predicates.end());
    return Predicate(&*up);
}

}
