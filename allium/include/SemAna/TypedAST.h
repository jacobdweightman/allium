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

size_t getConstructorIndex(const Type &type, const ConstructorRef &cr);

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


/*
 * Predicates
 */

struct UserPredicate;
struct BuiltinPredicate;

typedef TaggedUnion<
    const UserPredicate *,
    const BuiltinPredicate *
> PredicateBase;
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

struct EffectImplication {
    EffectImplication(EffectCtorRef head, Expression body):
        head(head), body(body) {}

    EffectCtorRef head;
    Expression body; // TODO: handler expression, which can include continuation?
};

bool operator==(const EffectImplication &left, const EffectImplication &right);
bool operator!=(const EffectImplication &left, const EffectImplication &right);

struct Handler {
    Handler(std::vector<EffectImplication> implications):
        implications(implications) {}

    std::vector<EffectImplication> implications;
};

bool operator==(const Handler &left, const Handler &right);
bool operator!=(const Handler &left, const Handler &right);

/// A predicate defined in user code.
struct UserPredicate {
    UserPredicate(
        PredicateDecl declaration,
        std::vector<Implication> implications,
        std::vector<Handler> handlers
    ): declaration(declaration), implications(implications),
        handlers(handlers) {}

    PredicateDecl declaration;
    std::vector<Implication> implications;
    std::vector<Handler> handlers;
};

/// Represents the effect of executing a builtin predicate on the groundness of
/// its arguments. Semantically, this is tied to Allium's left-to-right, depth-
/// first-search execution model.
struct Mode {
    Mode(std::vector<bool> in, std::vector<bool> out):
        inGroundness(in), outGroundness(out) {}
    std::vector<bool> inGroundness;
    std::vector<bool> outGroundness;
};

/// Represents a predicate which is hardcoded into the Allium interpreter. For
/// example, `concat` is a builtin predicate because the user doesn't have to
/// define it, and it is implemented in C++ inside the interpreter.
struct BuiltinPredicate {
    BuiltinPredicate(PredicateDecl declaration, std::vector<Mode> modes):
        declaration(declaration), modes(modes) {}

    /// A pointer to the coroutine which implements this predicate.
    PredicateDecl declaration;

    /// Represents all possible effects that executing this builtin predicate 
    /// might have on its arguments' groundness.
    std::vector<Mode> modes;
};

struct Predicate : public PredicateBase {
    using PredicateBase::PredicateBase;

    const PredicateDecl &getDeclaration() const {
        return this->match<const PredicateDecl&>(
        [](const UserPredicate *up) -> const PredicateDecl& { return up->declaration; },
        [](const BuiltinPredicate *bp) -> const PredicateDecl& { return bp->declaration; }
        );
    }
};

std::ostream& operator<<(std::ostream &out, const Value &val);

class AST {
public:
    AST(std::vector<Type> types,
        std::vector<Effect> effects,
        std::vector<UserPredicate> predicates
    ): types(types), effects(effects), predicates(predicates) {}

    const Type &resolveTypeRef(const Name<Type> &tr) const;

    const Constructor &resolveConstructorRef(const Name<Type> &tr, const ConstructorRef &cr) const;

    const Effect &resolveEffectRef(const Name<Effect> &er) const;

    const EffectCtor &resolveEffectCtorRef(const EffectCtorRef &ecr) const;

    const Predicate resolvePredicateRef(const PredicateRef &pr) const;

    std::vector<Type> types;
    std::vector<Effect> effects;
    std::vector<UserPredicate> predicates;
};

/// Represents the variables and their types defined in a scope.
typedef std::map<Name<Variable>, const Type *> Scope;

};

#endif // SEMANA_TYPED_AST_H
