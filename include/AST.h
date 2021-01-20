#ifndef AST_H
#define AST_H

#include <algorithm>
#include <assert.h>
#include <memory>
#include <vector>

#include "values/TaggedUnion.h"
#include "values/ParserValues.h"

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
struct ConstructorRef;
struct Value;
struct TypeRef;
struct Type;

std::ostream& operator<<(std::ostream &out, const Expression &e);

/// Represents a truth value literal in the AST.
struct TruthLiteral {
    TruthLiteral(): value(false), location(SourceLocation()) {}

    TruthLiteral(bool value, SourceLocation location):
        value(value), location(location) {}

    friend bool operator==(const TruthLiteral &lhs, const TruthLiteral &rhs) {
        return lhs.value == rhs.value && lhs.location == rhs.location;
    }

    friend inline bool operator!=(const TruthLiteral &lhs, const TruthLiteral &rhs) {
        return !(lhs == rhs);
    }

    bool value;
    SourceLocation location;
};

std::ostream& operator<<(std::ostream &out, const TruthLiteral &tl);

/// Represents the signature of a predicate at the start of its definition.
struct PredicateDecl {
    PredicateDecl() {}
    PredicateDecl(
        Name<Predicate> name,
        std::vector<TypeRef> parameters,
        SourceLocation location
    ): name(name), parameters(parameters), location(location) {}

    friend bool operator==(const PredicateDecl &lhs, const PredicateDecl &rhs) {
        return lhs.location == rhs.location && lhs.name == rhs.name &&
            lhs.parameters == rhs.parameters;
    }

    friend inline bool operator!=(const PredicateDecl &lhs, const PredicateDecl &rhs) {
        return !(lhs == rhs);
    }

    Name<Predicate> name;
    std::vector<TypeRef> parameters;
    SourceLocation location;
};

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

    friend bool operator==(const PredicateRef &lhs, const PredicateRef &rhs) {
        return lhs.name == rhs.name && lhs.location == rhs.location &&
            lhs.arguments == rhs.arguments;
    }

    friend bool operator!=(const PredicateRef &lhs, const PredicateRef &rhs) {
        return !(lhs == rhs);
    }

    Name<Predicate> name;
    std::vector<Value> arguments;
    SourceLocation location;
};

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

    friend bool operator==(const Conjunction &lhs, const Conjunction &rhs);
    friend bool operator!=(const Conjunction &lhs, const Conjunction &rhs);

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

    friend bool operator==(const Implication &lhs, const Implication &rhs) {
        return lhs.lhs == rhs.lhs && lhs.rhs == rhs.rhs;
    }

    friend inline bool operator!=(const Implication &lhs, const Implication &rhs) {
        return !(lhs == rhs);
    }

    PredicateRef lhs;
    Expression rhs;
};

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

    friend bool operator==(const Predicate &lhs, const Predicate &rhs) {
        return lhs.name == rhs.name && lhs.implications == rhs.implications;
    }

    friend inline bool operator!=(const Predicate &lhs, const Predicate &rhs) {
        return !(lhs == rhs);
    }

    PredicateDecl name;
    std::vector<Implication> implications;
};

std::ostream& operator<<(std::ostream &out, const Predicate &pred);

/// Represents a reference to a type in the AST, for example in a predicate's
/// signature definition.
struct TypeRef {
    TypeRef(): name(""), location(SourceLocation()) {}

    TypeRef(std::string name, SourceLocation location):
        name(name), location(location) {}

    TypeRef(const TypeRef &other):
        name(other.name), location(other.location) {}

    friend bool operator==(const TypeRef &lhs, const TypeRef &rhs) {
        return lhs.name == rhs.name && lhs.location == rhs.location;
    }

    friend bool operator!=(const TypeRef &lhs, const TypeRef &rhs) {
        return !(lhs == rhs);
    }

    Name<Type> name;
    SourceLocation location;
};

std::ostream& operator<<(std::ostream &out, const TypeRef &typeRef);

struct Constructor {
    Constructor() {}
    Constructor(
        std::string name,
        std::vector<TypeRef> parameters,
        SourceLocation location
    ): name(name), parameters(parameters), location(location) {}

    friend bool operator==(const Constructor &lhs, const Constructor &rhs) {
        return lhs.name == rhs.name && lhs.location == rhs.location;
    }

    friend bool operator!=(const Constructor &lhs, const Constructor &rhs) {
        return !(lhs == rhs);
    }

    Name<Constructor> name;
    std::vector<TypeRef> parameters;
    SourceLocation location;
};

std::ostream& operator<<(std::ostream &out, const Constructor &ctor);

struct AnonymousVariable {
    AnonymousVariable() {}
    AnonymousVariable(SourceLocation location): location(location) {}

    friend bool operator==(const AnonymousVariable &lhs, const AnonymousVariable &rhs) {
        return lhs.location == rhs.location;
    }

    friend bool operator!=(const AnonymousVariable &lhs, const AnonymousVariable &rhs) {
        return !(lhs == rhs);
    }

    SourceLocation location;
};

std::ostream& operator<<(std::ostream &out, const AnonymousVariable &av);

struct Variable {
    Variable() {}
    Variable(std::string name, bool isDefinition, SourceLocation location):
        name(name), isDefinition(isDefinition), location(location) {}
    
    friend bool operator==(const Variable &lhs, const Variable &rhs) {
        return lhs.name == rhs.name && lhs.location == rhs.location;
    }

    friend bool operator!=(const Variable &lhs, const Variable &rhs) {
        return !(lhs == rhs);
    }

    Name<Variable> name;
    bool isDefinition;
    SourceLocation location;
};

std::ostream& operator<<(std::ostream &out, const Variable &v);

struct ConstructorRef {
    ConstructorRef() {}
    ConstructorRef(std::string name, SourceLocation location):
        name(name), location(location) {}
    ConstructorRef(
        std::string name,
        std::vector<Value> arguments,
        SourceLocation location
    ): name(name), arguments(arguments), location(location) {}

    ConstructorRef operator=(ConstructorRef other) {
        using std::swap;
        swap(name, other.name);
        swap(arguments, other.arguments);
        swap(location, other.location);
        return *this;
    }

    friend bool operator==(const ConstructorRef &lhs, const ConstructorRef &rhs) {
        return lhs.name == rhs.name && lhs.location == rhs.location &&
            lhs.arguments == rhs.arguments;
    }

    friend bool operator!=(const ConstructorRef &lhs, const ConstructorRef &rhs) {
        return !(lhs == rhs);
    }

    Name<Constructor> name;
    std::vector<Value> arguments;
    SourceLocation location;
};

std::ostream& operator<<(std::ostream &out, const ConstructorRef &ctor);

typedef TaggedUnion<
    AnonymousVariable,
    Variable,
    ConstructorRef
> ValueBase;

struct Value : public ValueBase {
    using ValueBase::ValueBase;
    Value(): Value(AnonymousVariable()) {}
};

std::ostream& operator<<(std::ostream &out, const ConstructorRef &ctor);

/// Represents the declaration of a type at the begining of its definition.
struct TypeDecl {
    TypeDecl() {}
    TypeDecl(std::string name, SourceLocation location):
        name(name), location(location) {}

    friend bool operator==(const TypeDecl &lhs, const TypeDecl &rhs) {
        return lhs.name == rhs.name && lhs.location == rhs.location;
    }

    friend bool operator!=(const TypeDecl &lhs, const TypeDecl &rhs) {
        return !(lhs == rhs);
    }
    
    Name<Type> name;
    SourceLocation location;
};

std::ostream& operator<<(std::ostream &out, const TypeDecl &td);

/// Represents the complete definition of a type in the AST.
struct Type {
    Type() {}
    Type(TypeDecl declaration, std::vector<Constructor> ctors):
        declaration(declaration), constructors(ctors) {}
    
    friend bool operator==(const Type &lhs, const Type &rhs) {
        return lhs.declaration == rhs.declaration &&
            lhs.constructors == rhs.constructors;
    }

    friend bool operator!=(const Type &lhs, const Type &rhs) {
        return !(lhs == rhs);
    }

    TypeDecl declaration;
    std::vector<Constructor> constructors;
};

std::ostream& operator<<(std::ostream &out, const Type &type);

template <typename T>
class ASTVisitor {
public:
    virtual T visit(const TruthLiteral &tl) = 0;
    virtual T visit(const PredicateDecl &pd) = 0;
    virtual T visit(const PredicateRef &pr) = 0;
    virtual T visit(const Conjunction &conj) = 0;
    virtual T visit(const Expression &expr) = 0;
    virtual T visit(const Implication &impl) = 0;
    virtual T visit(const Predicate &p) = 0;
    virtual T visit(const TypeDecl &td) = 0;
    virtual T visit(const TypeRef &typeRef) = 0;
    virtual T visit(const Constructor &ctor) = 0;
    virtual T visit(const AnonymousVariable &av) = 0;
    virtual T visit(const Variable &v) = 0;
    virtual T visit(const ConstructorRef &ctor) = 0;
    virtual T visit(const Value &val) = 0;
    virtual T visit(const Type &type) = 0;
};

#endif // AST_H
