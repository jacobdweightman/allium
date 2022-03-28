#ifndef AST_H
#define AST_H

#include <algorithm>
#include <assert.h>
#include <memory>
#include <vector>

#include "Utils/Optional.h"
#include "Utils/ParserValues.h"
#include "Utils/TaggedUnion.h"

namespace parser {

struct TruthLiteral;
struct PredicateRef;
struct EffectCtorRef;
struct Conjunction;

/// Represents a logical expression in the AST.
typedef TaggedUnion<
    TruthLiteral,
    PredicateRef,
    EffectCtorRef,
    Conjunction
> ExpressionBase;
class Expression;

std::ostream& operator<<(std::ostream &out, const Expression &e);

struct NamedValue;
struct StringLiteral;
struct IntegerLiteral;

typedef TaggedUnion<
    NamedValue,
    StringLiteral,
    IntegerLiteral
> Value;

std::ostream& operator<<(std::ostream &out, const Value &val);

struct Predicate;
struct Type;
struct Parameter;

struct Effect;
struct EffectRef;
struct EffectConstructor;

struct Handler;
struct EffectImplication;

/// Represents a truth value literal in the AST.
struct TruthLiteral {
    TruthLiteral(): value(false), location(SourceLocation()) {}

    TruthLiteral(bool value, SourceLocation location):
        value(value), location(location) {}

    bool value;
    SourceLocation location;
};

bool operator==(const TruthLiteral &lhs, const TruthLiteral &rhs);
bool operator!=(const TruthLiteral &lhs, const TruthLiteral &rhs);
std::ostream& operator<<(std::ostream &out, const TruthLiteral &tl);

/// Represents the signature of a predicate at the start of its definition.
struct PredicateDecl {
    PredicateDecl() {}
    PredicateDecl(
        Name<Predicate> name,
        std::vector<Parameter> parameters,
        std::vector<EffectRef> effects,
        SourceLocation location
    ): name(name), parameters(parameters), effects(effects),
        location(location) {}

    Name<Predicate> name;
    std::vector<Parameter> parameters;
    std::vector<EffectRef> effects;
    SourceLocation location;
};

bool operator==(const PredicateDecl &lhs, const PredicateDecl &rhs);
bool operator!=(const PredicateDecl &lhs, const PredicateDecl &rhs);
std::ostream& operator<<(std::ostream &out, const PredicateDecl &pd);

/// Represents a reference to a predicate in the AST.
struct PredicateRef {
    PredicateRef() {}

    PredicateRef(std::string name, SourceLocation location):
        name(name), location(location) {}

    PredicateRef(
        std::string name,
        std::vector<Value> arguments,
        SourceLocation location
    ): name(name), arguments(arguments), location(location) {}

    Name<Predicate> name;
    std::vector<Value> arguments;
    SourceLocation location;
};

bool operator==(const PredicateRef &lhs, const PredicateRef &rhs);
bool operator!=(const PredicateRef &lhs, const PredicateRef &rhs);
std::ostream& operator<<(std::ostream &out, const PredicateRef &p);

/// Represents a concrete effect which should be performed when proving a
/// predicate.
struct EffectCtorRef {
    EffectCtorRef() {}

    EffectCtorRef(
        std::string name,
        std::vector<Value> arguments,
        SourceLocation location
    ): name(name), arguments(arguments), location(location) {}

    Name<EffectConstructor> name;
    std::vector<Value> arguments;
    SourceLocation location;
};

bool operator==(const EffectCtorRef &lhs, const EffectCtorRef &rhs);
bool operator!=(const EffectCtorRef &lhs, const EffectCtorRef &rhs);
std::ostream& operator<<(std::ostream &out, const EffectCtorRef &p);

/// Represents the conjunction of two expressions.
struct Conjunction {
public:
    Conjunction(Expression left, Expression right);
    Conjunction(const Conjunction &other);

    Conjunction operator=(Conjunction other) {
        using std::swap;
        swap(_left, other._left);
        swap(_right, other._right);
        return *this;
    }

    friend void swap(Conjunction &lhs, Conjunction &rhs) {
        using std::swap;
        swap(lhs._left, rhs._left);
        swap(lhs._right, rhs._right);
    }

    /// The left operand of the conjunction.
    ///
    /// For example, in the expression `A and B`, left is the subexpression `A`.
    Expression &getLeft() const {
        assert(_left && "left subexpression must not be null.");
        return *_left;
    }

    /// The right operand of the conjunction.
    ///
    /// For example, in the expression `A and B`, right is the subexpression `B`.
    Expression &getRight() const {
        assert(_right && "right subexpression must not be null.");
        return *_right;
    }

protected:
    // This is the underlying storage of the subexpressions of the conjunction.
    // To simplify memory management, this class makes copies of the subexpressions
    // with the same lifetime as the instance, but which can never be null.
    std::unique_ptr<Expression> _left, _right;
};

bool operator==(const Conjunction &lhs, const Conjunction &rhs);
bool operator!=(const Conjunction &lhs, const Conjunction &rhs);
std::ostream& operator<<(std::ostream &out, const Conjunction &conj);

class Expression : public ExpressionBase {
public:
    using ExpressionBase::ExpressionBase;
    Expression(): ExpressionBase(PredicateRef()) {}
};

struct Implication {
    Implication(): rhs(PredicateRef()) {}
    Implication(PredicateRef left, Expression right): lhs(left), rhs(right) {}

    Implication operator=(Implication other) {
        using std::swap;
        swap(lhs, other.lhs);
        swap(rhs, other.rhs);
        return *this;
    }

    PredicateRef lhs;
    Expression rhs;
};

bool operator==(const Implication &lhs, const Implication &rhs);
bool operator!=(const Implication &lhs, const Implication &rhs);
std::ostream& operator<<(std::ostream &out, const Implication &impl);

/// Represents a complete predicate definition in the AST.
struct Predicate {
    Predicate() {}
    Predicate(PredicateDecl name, std::vector<Implication> implications):
        name(name), implications(implications) {}

    Predicate operator=(Predicate other) {
        using std::swap;
        swap(name, other.name);
        swap(implications, other.implications);
        return *this;
    }

    PredicateDecl name;
    std::vector<Implication> implications;
    std::vector<Handler> handlers;
};

bool operator==(const Predicate &lhs, const Predicate &rhs);
inline bool operator!=(const Predicate &lhs, const Predicate &rhs);
std::ostream& operator<<(std::ostream &out, const Predicate &pred);

/// Represents the declaration of a type at the begining of its definition.
struct TypeDecl {
    TypeDecl() {}
    TypeDecl(std::string name, SourceLocation location):
        name(name), location(location) {}

    Name<Type> name;
    SourceLocation location;
};

bool operator==(const TypeDecl &lhs, const TypeDecl &rhs);
bool operator!=(const TypeDecl &lhs, const TypeDecl &rhs);
std::ostream& operator<<(std::ostream &out, const TypeDecl &td);

/// Represents a parameter to a type's constructor.
struct CtorParameter {
    CtorParameter(): name(""), location(SourceLocation()) {}

    CtorParameter(std::string name, SourceLocation location):
        name(name), location(location) {}

    Name<Type> name;
    SourceLocation location;
};

bool operator==(const CtorParameter &lhs, const CtorParameter &rhs);
bool operator!=(const CtorParameter &lhs, const CtorParameter &rhs);
std::ostream& operator<<(std::ostream &out, const CtorParameter &p);

struct Constructor {
    Constructor() {}
    Constructor(
        std::string name,
        std::vector<CtorParameter> parameters,
        SourceLocation location
    ): name(name), parameters(parameters), location(location) {}

    Name<Constructor> name;
    std::vector<CtorParameter> parameters;
    SourceLocation location;
};

bool operator==(const Constructor &lhs, const Constructor &rhs);
bool operator!=(const Constructor &lhs, const Constructor &rhs);
std::ostream& operator<<(std::ostream &out, const Constructor &ctor);


/// Represents values with a name and possibly arguments in the AST.
/// Semantically, this can be an anonymous variable, variable, or a constructor
/// reference. Syntactically, these cannot be reliably distinguished by a
/// context free grammar. Differentiating these nodes is performed by SemAna.
struct NamedValue {
    NamedValue() {}

    NamedValue(std::string name, bool isDefinition, SourceLocation location):
        name(name), isDefinition(isDefinition), location(location) {}

    NamedValue(
        std::string name,
        std::vector<Value> arguments,
        SourceLocation location
    ): name(name), isDefinition(false), arguments(arguments),
        location(location) {}

    Name<NamedValue> name;
    bool isDefinition;
    std::vector<Value> arguments;
    SourceLocation location;
};

bool operator==(const NamedValue &lhs, const NamedValue &rhs);
bool operator!=(const NamedValue &lhs, const NamedValue &rhs);
std::ostream& operator<<(std::ostream &out, const NamedValue &val);

struct StringLiteral {
    StringLiteral() {}
    StringLiteral(std::string text, SourceLocation location):
        text(text), location(location) {}

    std::string text;
    SourceLocation location;
};

bool operator==(const StringLiteral &lhs, const StringLiteral &rhs);
bool operator!=(const StringLiteral &lhs, const StringLiteral &rhs);
std::ostream& operator<<(std::ostream &out, const StringLiteral &str);

struct IntegerLiteral {
    IntegerLiteral() {}
    IntegerLiteral(int64_t value, SourceLocation location):
        value(value), location(location) {}

    int64_t value;
    SourceLocation location;
};

bool operator==(const IntegerLiteral &lhs, const IntegerLiteral &rhs);
bool operator!=(const IntegerLiteral &lhs, const IntegerLiteral &rhs);
std::ostream& operator<<(std::ostream &out, const IntegerLiteral &i);

/// Represents the complete definition of a type in the AST.
struct Type {
    Type() {}
    Type(TypeDecl declaration, std::vector<Constructor> ctors):
        declaration(declaration), constructors(ctors) {}

    TypeDecl declaration;
    std::vector<Constructor> constructors;
};

bool operator==(const Type &lhs, const Type &rhs);
bool operator!=(const Type &lhs, const Type &rhs);
std::ostream& operator<<(std::ostream &out, const Type &type);

/// Represents an "abstract" (without arguments) reference to an effect, as
/// would occur in a predicate declaration's effect list.
struct EffectRef {
    EffectRef(): name(""), location(SourceLocation()) {}

    EffectRef(std::string name, SourceLocation location):
        name(name), location(location) {}

    Name<Effect> name;
    SourceLocation location;
};

bool operator==(const EffectRef &lhs, const EffectRef &rhs);
bool operator!=(const EffectRef &lhs, const EffectRef &rhs);
std::ostream& operator<<(std::ostream &out, const EffectRef &e);

/// Represents the declaration of an effect at the begining of its definition.
struct EffectDecl {
    EffectDecl() {}
    EffectDecl(std::string name, SourceLocation location):
        name(name), location(location) {}

    Name<Effect> name;
    SourceLocation location;
};

bool operator==(const EffectDecl &lhs, const EffectDecl &rhs);
bool operator!=(const EffectDecl &lhs, const EffectDecl &rhs);
std::ostream& operator<<(std::ostream &out, const EffectDecl &type);

/// Represents a parameter, as would occur in a predicate or effect.
struct Parameter {
    Parameter(): name(""), isInputOnly(true), location(SourceLocation()) {}

    Parameter(std::string name, bool isInputOnly, SourceLocation location):
        name(name), isInputOnly(isInputOnly), location(location) {}

    /// The name of the parameter's type.
    Name<Type> name;

    /// True if this parameter's value must be an input; if false, then this
    /// parameter cannot a variable definition.
    /// TODO: are non-ground terms acceptable for `in` parameters?
    bool isInputOnly;

    SourceLocation location;
};

bool operator==(const Parameter &lhs, const Parameter &rhs);
bool operator!=(const Parameter &lhs, const Parameter &rhs);
std::ostream& operator<<(std::ostream &out, const Parameter &typeRef);

struct EffectConstructor {
    EffectConstructor() {}
    EffectConstructor(
        std::string name,
        std::vector<Parameter> parameters,
        SourceLocation location
    ): name(name), parameters(parameters), location(location) {}

    Name<EffectConstructor> name;
    std::vector<Parameter> parameters;
    SourceLocation location;
};

bool operator==(const EffectConstructor &lhs, const EffectConstructor &rhs);
bool operator!=(const EffectConstructor &lhs, const EffectConstructor &rhs);
std::ostream& operator<<(std::ostream &out, const EffectConstructor &type);

/// Represents the complete definition of an effect in the AST.
struct Effect {
    Effect() {}
    Effect(EffectDecl declaration, std::vector<EffectConstructor> ctors):
        declaration(declaration), constructors(ctors) {}

    EffectDecl declaration;
    std::vector<EffectConstructor> constructors;
};

bool operator==(const Effect &lhs, const Effect &rhs);
bool operator!=(const Effect &lhs, const Effect &rhs);
std::ostream& operator<<(std::ostream &out, const Effect &type);

// Represents the complete definition of an effect handler in the AST
struct Handler {
    Handler(EffectRef effect, std::vector<EffectImplication> implications):
        effect(effect), implications(implications) {}

    EffectRef effect;
    std::vector<EffectImplication> implications;
};

// Represents an individual effect implication in an effect handler
struct EffectImplication {
    EffectImplication() {}
    EffectImplication(EffectCtorRef ctor, Expression expression):
        ctor(ctor), expression(expression) {}

    EffectCtorRef ctor;
    Expression expression;
};

bool operator==(const Handler &lhs, const Handler &rhs);
bool operator!=(const Handler &lhs, const Handler &rhs);
std::ostream& operator<<(std::ostream &out, const Handler &type);

/// An AST representing a complete source file.
struct AST {
    AST() {}
    AST(std::vector<Type> types, std::vector<Effect> effects,
        std::vector<Predicate> predicates
    ): types(types), effects(effects), predicates(predicates) {}

    Optional<Type> resolveTypeRef(const Name<Type> &tr) const;
    Optional<const Effect*> resolveEffectRef(const EffectRef &er) const;
    Optional<Predicate> resolvePredicateRef(const PredicateRef &pr) const;

    std::vector<Type> types;
    std::vector<Effect> effects;
    std::vector<Predicate> predicates;
};

bool operator==(const AST &lhs, const AST &rhs);
bool operator!=(const AST &lhs, const AST &rhs);
std::ostream& operator<<(std::ostream &out, const AST &ast);

template <typename Visitor, typename Node>
struct has_visit {
    template <typename T>
    static constexpr
    decltype(std::declval<Visitor>().visit(std::declval<T>()), bool())
    find_method(int) {
        return true;
    }

    template <typename T>
    static constexpr bool find_method(...) {
        return false;
    }

    static constexpr bool value = find_method<Node>(0);
};

template <typename Subclass>
constexpr bool has_all_visitors() {
    static_assert(has_visit<Subclass, TruthLiteral>::value, "missing TruthLiteral visitor");
    static_assert(has_visit<Subclass, PredicateDecl>::value, "missing PredicateDecl visitor");
    static_assert(has_visit<Subclass, PredicateRef>::value, "missing PredicateRef visitor");
    static_assert(has_visit<Subclass, EffectCtorRef>::value, "missing EffectCtorRef visitor");
    static_assert(has_visit<Subclass, Conjunction>::value, "missing Conjunction visitor");
    static_assert(has_visit<Subclass, Expression>::value, "missing Expression visitor");
    static_assert(has_visit<Subclass, Implication>::value, "missing Implication visitor");
    static_assert(has_visit<Subclass, Predicate>::value, "missing Predicate visitor");
    static_assert(has_visit<Subclass, TypeDecl>::value, "missing TypeDecl visitor");
    static_assert(has_visit<Subclass, CtorParameter>::value, "missing Parameter visitor");
    static_assert(has_visit<Subclass, Constructor>::value, "missing Constructor visitor");
    static_assert(has_visit<Subclass, NamedValue>::value, "missing NamedValue visitor");
    static_assert(has_visit<Subclass, StringLiteral>::value, "missing StringLiteral visitor");
    static_assert(has_visit<Subclass, IntegerLiteral>::value, "missing IntegerLiteral visitor");
    static_assert(has_visit<Subclass, Value>::value, "missing Value visitor");
    static_assert(has_visit<Subclass, Type>::value, "missing Type visitor");
    static_assert(has_visit<Subclass, EffectRef>::value, "missing EffectRef visitor");
    static_assert(has_visit<Subclass, EffectDecl>::value, "missing EffectDecl visitor");
    static_assert(has_visit<Subclass, Parameter>::value, "missing Parameter visitor");
    static_assert(has_visit<Subclass, EffectConstructor>::value, "missing EffectConstructor visitor");
    static_assert(has_visit<Subclass, Effect>::value, "missing Effect visitor");
    static_assert(has_visit<Subclass, Handler>::value, "missing Handler visitor");

    return has_visit<Subclass, TruthLiteral>::value &&
        has_visit<Subclass, PredicateDecl>::value &&
        has_visit<Subclass, PredicateRef>::value &&
        has_visit<Subclass, EffectCtorRef>::value &&
        has_visit<Subclass, Conjunction>::value &&
        has_visit<Subclass, Expression>::value &&
        has_visit<Subclass, Implication>::value &&
        has_visit<Subclass, Predicate>::value &&
        has_visit<Subclass, TypeDecl>::value &&
        has_visit<Subclass, CtorParameter>::value &&
        has_visit<Subclass, Constructor>::value &&
        has_visit<Subclass, NamedValue>::value &&
        has_visit<Subclass, StringLiteral>::value &&
        has_visit<Subclass, IntegerLiteral>::value &&
        has_visit<Subclass, Value>::value &&
        has_visit<Subclass, Type>::value &&
        has_visit<Subclass, EffectRef>::value &&
        has_visit<Subclass, EffectDecl>::value &&
        has_visit<Subclass, Parameter>::value &&
        has_visit<Subclass, EffectConstructor>::value &&
        has_visit<Subclass, Effect>::value &&
        has_visit<Subclass, Handler>::value;
}

} // namespace parser

#endif // AST_H
