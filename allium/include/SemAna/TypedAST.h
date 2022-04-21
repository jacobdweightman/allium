#ifndef SEMANA_TYPED_AST_H
#define SEMANA_TYPED_AST_H

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "Utils/ParserValues.h"
#include "Utils/TaggedUnion.h"

/// The structs in this namespace represent the nodes of a fully resolved and
/// semantically valid AST.
///
/// Any "ambiguities" that require semantic information to resolve are resolved
/// during SemAna while raising the parser's AST to the typed AST.
namespace TypedAST {

/*
 * Types
 */

struct Type;
struct CtorParameter;

struct TypeDecl {
    TypeDecl() {}
    TypeDecl(std::string name): name(name) {}

    Name<Type> name;
};

bool operator==(const TypeDecl &left, const TypeDecl &right);
bool operator!=(const TypeDecl &left, const TypeDecl &right);

struct CtorParameter {
    CtorParameter(std::string type): type(type) {}

    Name<Type> type;
};

bool operator==(const CtorParameter &left, const CtorParameter &right);
bool operator!=(const CtorParameter &left, const CtorParameter &right);

struct Constructor {
    Constructor(std::string name, std::vector<CtorParameter> parameters):
        name(name), parameters(parameters) {}

    Name<Constructor> name;
    std::vector<CtorParameter> parameters;
};

bool operator==(const Constructor &left, const Constructor &right);
bool operator!=(const Constructor &left, const Constructor &right);

struct Type {
    Type(TypeDecl declaration, std::vector<Constructor> constructors):
        declaration(declaration), constructors(constructors) {}

    TypeDecl declaration;
    std::vector<Constructor> constructors;
};

bool operator==(const Type &left, const Type &right);
bool operator!=(const Type &left, const Type &right);


/*
 * Values
 */

struct AnonymousVariable;
struct Variable;
struct ConstructorRef;
struct StringLiteral;
struct IntegerLiteral;

typedef TaggedUnion<
    AnonymousVariable,
    Variable,
    ConstructorRef,
    StringLiteral,
    IntegerLiteral
> ValueBase;
class Value;

struct AnonymousVariable {
    AnonymousVariable(Name<Type> type): type(type) {}

    Name<Type> type;
};

bool operator==(AnonymousVariable left, AnonymousVariable right);
bool operator!=(AnonymousVariable left, AnonymousVariable right);

struct Variable {
    Variable(
        std::string name,
        Name<Type> type,
        bool isDefinition
    ): name(name), type(type), isDefinition(isDefinition) {}

    Name<Variable> name;
    Name<Type> type;
    bool isDefinition;
};

bool operator==(Variable left, Variable right);
bool operator!=(Variable left, Variable right);

struct ConstructorRef {
    ConstructorRef(std::string name, std::vector<Value> arguments);
    ConstructorRef(const ConstructorRef &other);

    Name<Constructor> name;
    std::vector<Value> arguments;
};

bool operator==(const ConstructorRef &left, const ConstructorRef &right);
bool operator!=(const ConstructorRef &left, const ConstructorRef &right);

/// Represents string literals, which are used to construct values of the
/// builtin type String.
struct StringLiteral {
    StringLiteral(std::string value): value(value) {}

    std::string value;
};

bool operator==(const StringLiteral &left, const StringLiteral &right);
bool operator!=(const StringLiteral &left, const StringLiteral &right);

struct IntegerLiteral {
    IntegerLiteral(int64_t value): value(value) {}

    std::int64_t value;
};

bool operator==(const IntegerLiteral &left, const IntegerLiteral &right);
bool operator!=(const IntegerLiteral &left, const IntegerLiteral &right);

class Value : public ValueBase {
public:
    using ValueBase::ValueBase;
};


/*
 * Effects
 */

struct Effect;
typedef Name<Type> EffectRef;

struct EffectDecl {
    EffectDecl() {}
    EffectDecl(std::string name): name(name) {}

    Name<Effect> name;
};

bool operator==(const EffectDecl &left, const EffectDecl &right);
bool operator!=(const EffectDecl &left, const EffectDecl &right);

struct Parameter {
    Parameter(std::string type, bool isInputOnly):
        type(type), isInputOnly(isInputOnly) {}

    Name<Type> type;
    bool isInputOnly;
};

bool operator==(const Parameter &left, const Parameter &right);
bool operator!=(const Parameter &left, const Parameter &right);

struct EffectCtor {
    EffectCtor(std::string name, std::vector<Parameter> parameters):
        name(name), parameters(parameters) {}

    Name<EffectCtor> name;
    std::vector<Parameter> parameters;
};

bool operator==(const EffectCtor &left, const EffectCtor &right);
bool operator!=(const EffectCtor &left, const EffectCtor &right);

struct Effect {
    Effect(EffectDecl declaration, std::vector<EffectCtor> constructors):
        declaration(declaration), constructors(constructors) {}

    EffectDecl declaration;
    std::vector<EffectCtor> constructors;
};

bool operator==(const Effect &left, const Effect &right);
bool operator!=(const Effect &left, const Effect &right);

// TODO: full implementation of handlers
struct Handler {};

bool operator==(const Handler &left, const Handler &right);
bool operator!=(const Handler &left, const Handler &right);


/*
 * Predicates
 */

struct Predicate;

struct TruthLiteral;
struct PredicateRef;
struct EffectCtorRef;
struct Conjunction;

typedef TaggedUnion<
    TruthLiteral,
    PredicateRef,
    EffectCtorRef,
    Conjunction
> Expression;

std::ostream& operator<<(std::ostream &out, const Expression &expr);

struct TruthLiteral {
    TruthLiteral(bool value): value(value) {}

    bool value;
};

std::ostream& operator<<(std::ostream &out, const TruthLiteral &tl);

struct PredicateDecl {
    PredicateDecl(
        std::string name,
        std::vector<Parameter> parameters,
        std::vector<EffectRef> effects
    ): name(name), parameters(parameters), effects(effects) {}

    Name<Predicate> name;
    std::vector<Parameter> parameters;
    std::vector<EffectRef> effects;
};

struct PredicateRef {
    PredicateRef(std::string name, std::vector<Value> arguments);

    Name<Predicate> name;
    std::vector<Value> arguments;
};

std::ostream& operator<<(std::ostream &out, const PredicateRef &pr);

struct EffectCtorRef {
    EffectCtorRef(
        std::string effectName,
        std::string ctorName,
        std::vector<Value> arguments,
        SourceLocation location
    ): effectName(effectName), ctorName(ctorName), arguments(arguments),
        location(location) {}

    Name<Effect> effectName;
    Name<EffectCtor> ctorName;
    std::vector<Value> arguments;
    SourceLocation location;
};

std::ostream& operator<<(std::ostream &out, const EffectCtorRef &ecr);

struct Conjunction {
    Conjunction(Expression left, Expression right);
    Conjunction(const Conjunction &other);

    Conjunction operator=(Conjunction other);

    Expression &getLeft() const;
    Expression &getRight() const;

protected:
    std::unique_ptr<Expression> _left, _right;
};

struct Implication {
    Implication(PredicateRef head, Expression body):
        head(head), body(body) {}

    PredicateRef head;
    Expression body;
};

std::ostream& operator<<(std::ostream &out, const Implication &impl);

struct Predicate {
    Predicate(
        PredicateDecl declaration,
        std::vector<Implication> implications,
        std::vector<Handler> handlers
    ): declaration(declaration), implications(implications),
        handlers(handlers) {}

    PredicateDecl declaration;
    std::vector<Implication> implications;
    std::vector<Handler> handlers;
};

std::ostream& operator<<(std::ostream &out, const Value &val);

class AST {
public:
    AST(std::vector<Type> types,
        std::vector<Effect> effects,
        std::vector<Predicate> predicates
    ): types(types), effects(effects), predicates(predicates) {}

    const Type &resolveTypeRef(const Name<Type> &tr) const {
        const auto x = std::find_if(
            types.begin(),
            types.end(),
            [&](const Type &type) { return type.declaration.name == tr; });

        assert(x != types.end());
        return *x;
    }

    const Constructor &resolveConstructorRef(const Name<Type> &tr, const ConstructorRef &cr) const {
        const Type &type = resolveTypeRef(tr);

        const auto ctor = std::find_if(
            type.constructors.begin(),
            type.constructors.end(),
            [&](const Constructor &ctor) { return ctor.name == cr.name; });

        assert(ctor != type.constructors.end());
        return *ctor;
    }

    const EffectCtor &resolveEffectCtorRef(const EffectCtorRef &ecr) const {
        const auto effect = std::find_if(
            effects.begin(),
            effects.end(),
            [&](const Effect &e) { return e.declaration.name == ecr.effectName; });
        
        assert(effect != effects.end());

        const auto eCtor = std::find_if(
            effect->constructors.begin(),
            effect->constructors.end(),
            [&](const EffectCtor &eCtor) { return eCtor.name == ecr.ctorName; });
        
        assert(eCtor != effect->constructors.end());
        return *eCtor;
    }

    const Predicate &resolvePredicateRef(const PredicateRef &pr) const {
        const auto p = std::find_if(
            predicates.begin(),
            predicates.end(),
            [&](const Predicate &p) { return p.declaration.name == pr.name; });

        assert(p != predicates.end());
        return *p;
    }

    std::vector<Type> types;
    std::vector<Effect> effects;
    std::vector<Predicate> predicates;
};

/// Represents the variables and their types defined in a scope.
typedef std::map<Name<Variable>, const Type &> Scope;

};

#endif // SEMANA_TYPED_AST_H
