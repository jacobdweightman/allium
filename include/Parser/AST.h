#ifndef AST_H
#define AST_H

#include <algorithm>
#include <assert.h>
#include <memory>
#include <vector>

#include "values/Optional.h"
#include "values/TaggedUnion.h"
#include "values/ParserValues.h"

namespace parser {

struct TruthLiteral;
struct PredicateRef;
struct Conjunction;

/// Represents a logical expression in the AST.
typedef TaggedUnion<
    TruthLiteral,
    PredicateRef,
    Conjunction
> ExpressionBase;
class Expression;

struct Predicate;
struct Value;
struct TypeRef;
struct Type;

std::ostream& operator<<(std::ostream &out, const Expression &e);

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
        std::vector<TypeRef> parameters,
        SourceLocation location
    ): name(name), parameters(parameters), location(location) {}

    Name<Predicate> name;
    std::vector<TypeRef> parameters;
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

/// Represents a reference to a type in the AST, for example in a predicate's
/// signature definition.
struct TypeRef {
    TypeRef(): name(""), location(SourceLocation()) {}

    TypeRef(std::string name, SourceLocation location):
        name(name), location(location) {}

    TypeRef(const TypeRef &other):
        name(other.name), location(other.location) {}

    Name<Type> name;
    SourceLocation location;
};

bool operator==(const TypeRef &lhs, const TypeRef &rhs);
bool operator!=(const TypeRef &lhs, const TypeRef &rhs);
std::ostream& operator<<(std::ostream &out, const TypeRef &typeRef);

struct Constructor {
    Constructor() {}
    Constructor(
        std::string name,
        std::vector<TypeRef> parameters,
        SourceLocation location
    ): name(name), parameters(parameters), location(location) {}

    Name<Constructor> name;
    std::vector<TypeRef> parameters;
    SourceLocation location;
};

bool operator==(const Constructor &lhs, const Constructor &rhs);
bool operator!=(const Constructor &lhs, const Constructor &rhs);
std::ostream& operator<<(std::ostream &out, const Constructor &ctor);

/// Represents a value in the AST. Semantically, this can be an anonymous
/// variable, variable, or a constructor reference. Syntactically, these cannot
/// be reliably distinguished by a context free grammar. Differentiating these
/// nodes is performed by SemAna.
struct Value {
    Value(std::string name, bool isDefinition, SourceLocation location):
        name(name), isDefinition(isDefinition), location(location) {}

    Value(std::string name, std::vector<Value> arguments, SourceLocation location):
        name(name), isDefinition(false), arguments(arguments), location(location) {}

    Name<Value> name;
    bool isDefinition;
    std::vector<Value> arguments;
    SourceLocation location;
};

bool operator==(const Value &lhs, const Value &rhs);
bool operator!=(const Value &lhs, const Value &rhs);
std::ostream& operator<<(std::ostream &out, const Value &val);

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

struct Effect;

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

struct EffectConstructor {
    EffectConstructor() {}
    EffectConstructor(
        std::string name,
        std::vector<TypeRef> parameters,
        SourceLocation location
    ): name(name), parameters(parameters), location(location) {}

    Name<EffectConstructor> name;
    std::vector<TypeRef> parameters;
    SourceLocation location;
};

bool operator==(const EffectConstructor &lhs, const EffectConstructor &rhs);
bool operator!=(const EffectConstructor &lhs, const EffectConstructor &rhs);
std::ostream& operator<<(std::ostream &out, const EffectConstructor &type);

/// Represents the complete definition of a type in the AST.
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

/// An AST representing a complete source file.
struct AST {
    AST() {}
    AST(std::vector<Type> types, std::vector<Effect> effects, 
        std::vector<Predicate> predicates
    ): types(types), effects(effects), predicates(predicates) {}

    Optional<Type> resolveTypeRef(const TypeRef &tr) const;
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
    static_assert(has_visit<Subclass, TruthLiteral>::value);
    static_assert(has_visit<Subclass, PredicateDecl>::value);
    static_assert(has_visit<Subclass, PredicateRef>::value);
    static_assert(has_visit<Subclass, Conjunction>::value);
    static_assert(has_visit<Subclass, Expression>::value);
    static_assert(has_visit<Subclass, Implication>::value);
    static_assert(has_visit<Subclass, Predicate>::value);
    static_assert(has_visit<Subclass, TypeDecl>::value);
    static_assert(has_visit<Subclass, TypeRef>::value);
    static_assert(has_visit<Subclass, Constructor>::value);
    static_assert(has_visit<Subclass, Value>::value);
    static_assert(has_visit<Subclass, Type>::value);
    static_assert(has_visit<Subclass, EffectDecl>::value);
    static_assert(has_visit<Subclass, EffectConstructor>::value);
    static_assert(has_visit<Subclass, Effect>::value);

    return has_visit<Subclass, TruthLiteral>::value &&
        has_visit<Subclass, PredicateDecl>::value &&
        has_visit<Subclass, PredicateRef>::value &&
        has_visit<Subclass, Conjunction>::value &&
        has_visit<Subclass, Expression>::value &&
        has_visit<Subclass, Implication>::value &&
        has_visit<Subclass, Predicate>::value &&
        has_visit<Subclass, TypeDecl>::value &&
        has_visit<Subclass, TypeRef>::value &&
        has_visit<Subclass, Constructor>::value &&
        has_visit<Subclass, Value>::value &&
        has_visit<Subclass, Type>::value &&
        has_visit<Subclass, EffectDecl>::value &&
        has_visit<Subclass, EffectConstructor>::value &&
        has_visit<Subclass, Effect>::value;
}

} // namespace parser

#endif // AST_H
