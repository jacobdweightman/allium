#ifndef SEMANA_TYPED_AST_H
#define SEMANA_TYPED_AST_H

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "values/ParserValues.h"
#include "values/TaggedUnion.h"

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
typedef Name<Type> TypeRef;

struct TypeDecl {
    TypeDecl() {}
    TypeDecl(std::string name): name(name) {}

    Name<Type> name;
};

bool operator==(const TypeDecl &left, const TypeDecl &right);
bool operator!=(const TypeDecl &left, const TypeDecl &right);

struct Constructor {
    Constructor(std::string name, std::vector<TypeRef> parameters):
        name(name), parameters(parameters) {}

    Name<Constructor> name;
    std::vector<TypeRef> parameters;
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

typedef TaggedUnion<
    AnonymousVariable,
    Variable,
    ConstructorRef
> ValueBase;
class Value;

struct AnonymousVariable {
    AnonymousVariable() {}

    friend bool operator==(AnonymousVariable left, AnonymousVariable right) { return true; }
    friend bool operator!=(AnonymousVariable left, AnonymousVariable right) { return false; }
};

struct Variable {
    Variable(
        std::string name,
        TypeRef type,
        bool isDefinition,
        bool isExistential
    ): name(name), type(type), isDefinition(isDefinition),
        isExistential(isExistential) {}

    Name<Variable> name;
    TypeRef type;
    bool isDefinition;
    bool isExistential;
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

class Value : public ValueBase {
public:
    using ValueBase::ValueBase;
};


/*
 * Predicates
 */

struct Predicate;

struct TruthLiteral;
struct PredicateRef;
struct Conjunction;

typedef TaggedUnion<
    TruthLiteral,
    PredicateRef,
    Conjunction
> Expression;

struct TruthLiteral {
    TruthLiteral(bool value): value(value) {}

    bool value;
};

struct PredicateDecl {
    PredicateDecl(std::string name, std::vector<TypeRef> parameters):
        name(name), parameters(parameters) {}

    Name<Predicate> name;
    std::vector<TypeRef> parameters;
};

struct PredicateRef {
    PredicateRef(std::string name, std::vector<Value> arguments);

    Name<Predicate> name;
    std::vector<Value> arguments;
};

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

struct Predicate {
    Predicate(PredicateDecl declaration, std::vector<Implication> implications):
        declaration(declaration), implications(implications) {}

    PredicateDecl declaration;
    std::vector<Implication> implications;
};

std::ostream& operator<<(std::ostream &out, const Value &val);
std::ostream& operator<<(std::ostream &out, const PredicateRef &pr);

class AST {
public:
    AST(std::vector<Type> types,
        std::vector<Predicate> predicates
    ): types(types), predicates(predicates) {}

    const Type &resolveTypeRef(const TypeRef &tr) const {
        const auto x = std::find_if(
            types.begin(),
            types.end(),
            [&](const Type &type) { return type.declaration.name == tr; });

        assert(x != types.end());
        return *x;
    }

    const Constructor &resolveConstructorRef(const TypeRef &tr, const ConstructorRef &cr) const {
        const Type &type = resolveTypeRef(tr);

        const auto ctor = std::find_if(
            type.constructors.begin(),
            type.constructors.end(),
            [&](const Constructor &ctor) { return ctor.name == cr.name; });

        assert(ctor != type.constructors.end());
        return *ctor;
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
    std::vector<Predicate> predicates;
};

struct VariableInfo {
    VariableInfo(
        Type type,
        bool isExistential
    ): type(type), isExistential(isExistential) {}

    Type type;
    bool isExistential;
};

/// Represents the variables and their types defined in a scope.
typedef std::map<Name<Variable>, VariableInfo> Scope;

};

#endif // SEMANA_TYPED_AST_H
