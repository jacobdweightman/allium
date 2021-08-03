#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <algorithm>
#include <assert.h>
#include <limits>
#include <memory>
#include <vector>

#include "Utils/TaggedUnion.h"

// Define the value types of the language which are used by the interpreter.
// The AST is lowered into these types after semantic analysis so that the
// program may be interpretted.
//
// It is desirable to decouple these types for several reasons:
//   1. They represent fundamentally different concepts which cannot be
//      confused thanks to the type checker.
//   2. We enforce a strict separation of lexical and execution information
//      which will make lowering to LLVM IR easier in the future.
//   3. Once semantic analysis has completed we can discard all syntactic
//      information to reduce memory consumption.

namespace interpreter {

struct TruthValue;
struct PredicateReference;
struct EffectCtorRef;
struct Conjunction;

typedef TaggedUnion<
    TruthValue,
    PredicateReference,
    EffectCtorRef,
    Conjunction
> Expression;

std::ostream& operator<<(std::ostream &out, const Expression &expr);

class Value;

struct ConstructorRef {
    ConstructorRef(): index(std::numeric_limits<size_t>::max()) {}
    ConstructorRef(size_t index, std::vector<Value> arguments):
        index(index), arguments(arguments) {}

    friend bool operator==(const ConstructorRef &lhs, const ConstructorRef &rhs) {
        return lhs.index == rhs.index && lhs.arguments == rhs.arguments;
    }

    friend bool operator!=(const ConstructorRef &lhs, const ConstructorRef &rhs) {
        return !(lhs == rhs);
    }

    size_t index;
    std::vector<Value> arguments;
};

std::ostream& operator<<(std::ostream &out, const ConstructorRef &ctor);

/// Represents a value of the builtin type String.
struct String {
    String(std::string str): value(str) {}

    friend bool operator==(const String &lhs, const String &rhs) {
        return lhs.value == rhs.value;
    }

    friend bool operator!=(const String &lhs, const String &rhs) {
        return !(lhs == rhs);
    }

    std::string value;
};

std::ostream& operator<<(std::ostream &out, const String &str);

struct VariableRef {
    VariableRef(bool isTypeInhabited = true):
        index(anonymousIndex), isDefinition(false), isExistential(false),
        isTypeInhabited(isTypeInhabited) {}
    VariableRef(
        size_t index,
        bool isDefinition,
        bool isExistential,
        bool isTypeInhabited = true
    ): index(index), isDefinition(isDefinition), isExistential(isExistential),
        isTypeInhabited(isTypeInhabited) {}

    friend bool operator==(const VariableRef &lhs, const VariableRef &rhs) {
        return lhs.index == rhs.index && lhs.isDefinition == rhs.isDefinition &&
            lhs.isExistential == rhs.isExistential &&
            lhs.isTypeInhabited == rhs.isTypeInhabited;
    }

    friend bool operator!=(const VariableRef &lhs, const VariableRef &rhs) {
        return !(lhs == rhs);
    }

    static const size_t anonymousIndex = std::numeric_limits<size_t>::max();

    /// The index of this variable within the witness producer's variable table.
    size_t index;

    /// Whether or not the variable is defined by this reference.
    bool isDefinition;

    /// True if the variable is existentially quantified, false if it is
    /// universally quantified.
    bool isExistential;

    /// Whether or not this variable's type is inhabited. In an existence proof
    /// where a variable is never bound, we must not produce a witness of an
    /// uninhabited type. This information must be propagated from the TypedAST.
    bool isTypeInhabited;
};

std::ostream& operator<<(std::ostream &out, const VariableRef &vr);

class Value : public TaggedUnion<ConstructorRef, String, Value *, VariableRef> {
public:
    using ValueBase = TaggedUnion<
        ConstructorRef,
        String,
        Value *,
        VariableRef
    >;
    using ValueBase::ValueBase;
    Value(): ValueBase(VariableRef()) {}
};

std::ostream& operator<<(std::ostream &out, const Value &val);

struct EffectCtorRef {
    EffectCtorRef(
        size_t effectIndex,
        size_t effectCtorIndex,
        std::vector<Value> arguments
    ): effectIndex(effectIndex), effectCtorIndex(effectCtorIndex),
        arguments(arguments) {}

    friend bool operator==(const EffectCtorRef &lhs, const EffectCtorRef &rhs) {
        return lhs.effectIndex == rhs.effectIndex &&
            lhs.effectCtorIndex == rhs.effectCtorIndex &&
            lhs.arguments == rhs.arguments;
    }

    friend bool operator!=(const EffectCtorRef &lhs, const EffectCtorRef &rhs) {
        return !(lhs == rhs);
    }

    /// A number which uniquely identifies the effect type. This is used to
    /// lookup handlers in the interpreter.
    size_t effectIndex;

    /// A number which uniquely identifies this effect's constructor. This is
    /// necessary for pattern-matching in effect handlers.
    size_t effectCtorIndex;

    /// The arguments which should be passed to the effect handler.
    std::vector<Value> arguments;
};

std::ostream& operator<<(std::ostream &out, const EffectCtorRef &ecr);

struct TruthValue {
    TruthValue(bool value): value(value) {}

    friend bool operator==(const TruthValue &left, const TruthValue &right) {
        return left.value == right.value;
    }

    friend bool operator!=(const TruthValue &left, const TruthValue &right) {
        return !(left == right);
    }

    bool value;
};

std::ostream& operator<<(std::ostream &out, const TruthValue &tv);

struct PredicateReference {
    PredicateReference(size_t index, std::vector<Value> arguments): 
        index(index), arguments(arguments) {}

    friend bool operator==(const PredicateReference &left, const PredicateReference &right) {
        return left.index == right.index && left.arguments == right.arguments;
    }

    friend bool operator!=(const PredicateReference &left, const PredicateReference &right) {
        return !(left == right);
    }
    
    /// The index into the program's predicate list.
    size_t index;

    std::vector<Value> arguments;
};

std::ostream& operator<<(std::ostream &out, const PredicateReference &pr);

struct Conjunction {
    Conjunction(Expression left, Expression right):
        left(new auto(left)), right(new auto(right)) {}

    Conjunction(const Conjunction &other):
        left(new auto(*other.left)), right(new auto(*other.right)) {}

    Conjunction operator=(Conjunction other) {
        using std::swap;
        swap(left, other.left);
        swap(right, other.right);
        return *this;
    }

    friend bool operator==(const Conjunction &left, const Conjunction &right) {
        return left.getLeft() == right.getLeft() && left.getRight() == right.getRight();
    }

    friend bool operator!=(const Conjunction &left, const Conjunction &right) {
        return !(left == right);
    }

    const Expression &getLeft() const { return *left; }
    const Expression &getRight() const { return *right; }
private:
    std::unique_ptr<Expression> left, right;
};

std::ostream& operator<<(std::ostream &out, const Conjunction &conj);

struct Implication {
    Implication(
        PredicateReference head, Expression body,
        size_t variableCount
    ): head(head), body(body), variableCount(variableCount) {}

    Implication operator=(Implication other) {
        using std::swap;
        swap(head, other.head);
        swap(body, other.body);
        swap(variableCount, other.variableCount);
        return *this;
    }

    PredicateReference head;
    Expression body;
    size_t variableCount;
};

bool operator==(const Implication &, const Implication &);
bool operator!=(const Implication &, const Implication &);
std::ostream& operator<<(std::ostream &out, const Implication &impl);

struct Predicate {
    Predicate(std::vector<Implication> implications):
        implications(implications) {}

    Predicate operator=(Predicate other) {
        swap(implications, other.implications);
        return *this;
    }

    std::vector<Implication> implications;
};

bool operator==(const Predicate &, const Predicate &);
bool operator!=(const Predicate &, const Predicate &);
std::ostream& operator<<(std::ostream &out, const Predicate &p);

// A container for configuration parameters of the program.
struct Config {
    enum class LogLevel {
        OFF = 0,
        QUIET = 1,
        LOUD = 2,
        MAX = 3,
    };

    LogLevel debugLevel = LogLevel::OFF;
};

class Program {
public:
    Program(
        std::vector<Predicate> ps,
        Optional<PredicateReference> main,
        Config config = Config()
    ): predicates(ps), entryPoint(main), config(config) {}
    
    friend bool operator==(const Program &, const Program &);
    friend bool operator!=(const Program &, const Program &);
    friend std::ostream& operator<<(std::ostream &out, const Program &prog);

    bool prove(const Expression&);

    const Optional<PredicateReference> &getEntryPoint() {
        return entryPoint;
    }

    const Predicate &getPredicate(size_t index) const {
        assert(index < predicates.size());
        return predicates[index];
    }

    /// The program's configuration parameters.
    const Config config;

protected:
    bool match(
        const Implication &impl,
        const VariableRef &vr,
        const ConstructorRef &cr) const;
    bool match(const VariableRef &vl, const VariableRef &vr) const;
    bool match(const ConstructorRef &cl, const ConstructorRef &cr) const;
    bool match(const Value &left, const Value &right) const;

    /// A collection of the predicates defined in the program. Predicates
    /// refer to each other through their indices in this vector.
    std::vector<Predicate> predicates;

    /// The predicate which represents whether the program accepts or
    /// rejects, if there is one, i.e. `main`.
    ///
    /// There may be no such value if the program doesn't define `main`.
    /// Such a program always rejects.
    Optional<PredicateReference> entryPoint;
};

} // namespace interpreter

#endif // INTERPRETER_H
