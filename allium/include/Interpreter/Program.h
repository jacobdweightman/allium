#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <algorithm>
#include <assert.h>
#include <limits>
#include <memory>
#include <vector>

#include "Utils/Generator.h"
#include "Utils/TaggedUnion.h"
#include "Utils/Unit.h"

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

#ifdef _MSC_VER
#define PUBLIC_GLOBAL __declspec(dllimport)
#else
#define PUBLIC_GLOBAL
#endif

namespace interpreter {

class Program;

struct TruthValue;
struct Continuation;
struct PredicateReference;
struct BuiltinPredicateReference;
struct EffectCtorRef;
struct Conjunction;
struct HandlerConjunction;

typedef TaggedUnion<
    TruthValue,
    PredicateReference,
    BuiltinPredicateReference,
    EffectCtorRef,
    Conjunction
> Expression;

std::ostream& operator<<(std::ostream &out, const Expression &expr);

typedef TaggedUnion<
    TruthValue,
    Continuation,
    PredicateReference,
    BuiltinPredicateReference,
    EffectCtorRef,
    HandlerConjunction
> HandlerExpression;

std::ostream& operator<<(std::ostream &out, const HandlerExpression &hExpr);

class MatcherValue;
class RuntimeValue;

/// Represents a constructor value for the sake of pattern matching.
struct MatcherCtorRef {
    MatcherCtorRef(): index(std::numeric_limits<size_t>::max()) {}
    MatcherCtorRef(size_t index, std::vector<MatcherValue> arguments):
        index(index), arguments(arguments) {}

    friend bool operator==(const MatcherCtorRef &lhs, const MatcherCtorRef &rhs) {
        return lhs.index == rhs.index && lhs.arguments == rhs.arguments;
    }

    friend bool operator!=(const MatcherCtorRef &lhs, const MatcherCtorRef &rhs) {
        return !(lhs == rhs);
    }

    size_t index;
    std::vector<MatcherValue> arguments;
};

std::ostream& operator<<(std::ostream &out, const MatcherCtorRef &mCtor);

/// Represents a variable value for the sake of pattern matching.
struct MatcherVariable {
    MatcherVariable(
        size_t index,
        bool isTypeInhabited = true
    ): index(index), isTypeInhabited(isTypeInhabited) {}

    friend bool operator==(const MatcherVariable &lhs, const MatcherVariable &rhs) {
        return lhs.index == rhs.index &&
            lhs.isTypeInhabited == rhs.isTypeInhabited;
    }

    friend bool operator!=(const MatcherVariable &lhs, const MatcherVariable &rhs) {
        return !(lhs == rhs);
    }

    static const size_t anonymousIndex = std::numeric_limits<size_t>::max();

    /// The index of this variable within the witness producer's variable table.
    size_t index;

    /// Whether or not this variable's type is inhabited. In an existence proof
    /// where a variable is never bound, we must not produce a witness of an
    /// uninhabited type. This information must be propagated from the TypedAST.
    bool isTypeInhabited;
};

std::ostream& operator<<(std::ostream &out, const MatcherVariable &mv);

/// Represents a constructor value which could be the value of a variable.
struct RuntimeCtorRef {
    RuntimeCtorRef(): index(std::numeric_limits<size_t>::max()) {}
    RuntimeCtorRef(size_t index, std::vector<RuntimeValue> arguments):
        index(index), arguments(arguments) {}

    friend bool operator==(const RuntimeCtorRef &lhs, const RuntimeCtorRef &rhs) {
        return lhs.index == rhs.index && lhs.arguments == rhs.arguments;
    }

    friend bool operator!=(const RuntimeCtorRef &lhs, const RuntimeCtorRef &rhs) {
        return !(lhs == rhs);
    }

    size_t index;
    std::vector<RuntimeValue> arguments;
};

std::ostream& operator<<(std::ostream &out, const RuntimeCtorRef &ctor);

/// Represents a value of the builtin type String.
struct String {
    String() {}
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

/// Represents a value of the builtin type Int.
struct Int {
    Int(int64_t value): value(value) {}

    friend bool operator==(const Int &lhs, const Int &rhs) {
        return lhs.value == rhs.value;
    }

    friend bool operator!=(const Int &lhs, const Int &rhs) {
        return !(lhs == rhs);
    }

    int64_t value;
};

std::ostream& operator<<(std::ostream &out, const Int &i);

typedef TaggedUnion<
    std::monostate,
    RuntimeCtorRef,
    String,
    Int,
    RuntimeValue *
> RuntimeValueBase;

class RuntimeValue : public RuntimeValueBase {
public:
    using RuntimeValueBase::RuntimeValueBase;
    RuntimeValue(): RuntimeValueBase(std::monostate {}) {}

    constexpr bool isDefined() const {
        return wrapped.index() != 0;
    }

    /// Follows a chain of pointers-to-values until it finds a non-pointer value.
    RuntimeValue &getValue();

    friend bool operator==(const RuntimeValue &lhs, const RuntimeValue &rhs) {
        return lhs.wrapped == rhs.wrapped;
    }

    friend bool operator!=(const RuntimeValue &lhs, const RuntimeValue &rhs) {
        return !(lhs == rhs);
    }
};

std::ostream& operator<<(std::ostream &out, const RuntimeValue &val);

PUBLIC_GLOBAL extern RuntimeValue *uninhabitedTypeVar;

inline bool isVarTypeUninhabited(RuntimeValue *var) {
    return var == uninhabitedTypeVar;
}

inline bool isAnonymousVariable(RuntimeValue *var) {
    return var == nullptr;
}

/// Represents the values of all variables local to a particular context.
typedef std::vector<RuntimeValue> Context;

typedef TaggedUnion<
    std::monostate,
    MatcherCtorRef,
    String,
    Int,
    MatcherVariable
> MatcherValueBase;

class MatcherValue : public MatcherValueBase {
public:
    using MatcherValueBase::MatcherValueBase;
    MatcherValue(): MatcherValueBase(std::monostate {}) {}

    RuntimeValue lower(Context &context) const;

    friend bool operator==(const MatcherValue &lhs, const MatcherValue &rhs) {
        return lhs.wrapped == rhs.wrapped;
    }

    friend bool operator!=(const MatcherValue &lhs, const MatcherValue &rhs) {
        return !(lhs == rhs);
    }
};

std::ostream& operator<<(std::ostream &out, const MatcherValue &val);

struct EffectCtorRef {
    EffectCtorRef(
        size_t effectIndex,
        size_t effectCtorIndex,
        std::vector<MatcherValue> arguments,
        Expression continuation);

    EffectCtorRef(const EffectCtorRef &other);

    EffectCtorRef& operator=(EffectCtorRef other);

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
    std::vector<MatcherValue> arguments;

    /// The continuation of the effect, which may be invoked by the handler.
    Expression& getContinuation() const;

private:
    std::unique_ptr<Expression> _continuation;
};

std::ostream& operator<<(std::ostream &out, const EffectCtorRef &ecr);

struct EffectImplHead {
    EffectImplHead(
        size_t effectIndex,
        size_t effectCtorIndex,
        std::vector<MatcherValue> arguments
    ): effectIndex(effectIndex), effectCtorIndex(effectCtorIndex),
        arguments(arguments) {}

    /// A number which uniquely identifies the effect type. This is used to
    /// lookup handlers in the interpreter.
    size_t effectIndex;

    /// A number which uniquely identifies this effect's constructor. This is
    /// necessary for pattern-matching in effect handlers.
    size_t effectCtorIndex;

    /// The arguments which should be passed to the effect handler.
    std::vector<MatcherValue> arguments;
};

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

struct Continuation {
    Continuation() {}
};

bool operator==(const Continuation &, const Continuation &);
bool operator!=(const Continuation &, const Continuation &);
std::ostream& operator<<(std::ostream &out, const Continuation &k);

struct PredicateReference {
    PredicateReference(size_t index, std::vector<MatcherValue> arguments): 
        index(index), arguments(arguments) {}

    friend bool operator==(const PredicateReference &left, const PredicateReference &right) {
        return left.index == right.index && left.arguments == right.arguments;
    }

    friend bool operator!=(const PredicateReference &left, const PredicateReference &right) {
        return !(left == right);
    }
    
    /// The index into the program's predicate list.
    size_t index;

    std::vector<MatcherValue> arguments;
};

std::ostream& operator<<(std::ostream &out, const PredicateReference &pr);

typedef Generator<Unit> (*BuiltinPredicate)(std::vector<RuntimeValue>);

/// Represents a reference to a builtin predicate.
struct BuiltinPredicateReference {
    BuiltinPredicateReference(BuiltinPredicate p, std::vector<MatcherValue> arguments):
        predicate(p), arguments(arguments) {}

    friend bool operator==(const BuiltinPredicateReference &left, const BuiltinPredicateReference &right) {
        return left.predicate == right.predicate && left.arguments == right.arguments;
    }

    friend bool operator!=(const BuiltinPredicateReference &left, const BuiltinPredicateReference &right) {
        return !(left == right);
    }

    /// A pointer to the predicate's implementation.
    BuiltinPredicate predicate;

    /// The predicate's arguments.
    std::vector<MatcherValue> arguments;
};

std::ostream& operator<<(std::ostream &out, const BuiltinPredicateReference &bpr);

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

struct HandlerConjunction {
    HandlerConjunction(HandlerExpression left, HandlerExpression right);
    HandlerConjunction(const HandlerConjunction &other);
    HandlerConjunction operator=(HandlerConjunction other);

    const HandlerExpression &getLeft() const { return *left; }
    const HandlerExpression &getRight() const { return *right; }
private:
    std::unique_ptr<HandlerExpression> left, right;
};

bool operator==(const HandlerConjunction &left, const HandlerConjunction &right);
bool operator!=(const HandlerConjunction &left, const HandlerConjunction &right);
std::ostream& operator<<(std::ostream &out, const HandlerConjunction &hConj);

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

struct EffectImplication {
    EffectImplication(
        EffectImplHead head,
        HandlerExpression body,
        size_t variableCount
    ): head(head), body(body), variableCount(variableCount) {}

    EffectImplHead head;
    HandlerExpression body;
    size_t variableCount;
};

bool operator==(const EffectImplication &, const EffectImplication &);
bool operator!=(const EffectImplication &, const EffectImplication &);
std::ostream& operator<<(std::ostream &out, const EffectImplication &eImpl);

struct Handler;
using HandlerStack = std::vector<Handler>;

typedef Generator<Unit> (*BuiltinHandler)(
    const Program &prog,
    const EffectCtorRef &ecr,
    Context &context,
    HandlerStack &handlers);

struct UserHandler {
    UserHandler(size_t effect, std::vector<EffectImplication> implications):
        effect(effect), implications(implications) {}

    /// A number which uniquely identifies the effect type that this handler
    /// handles.
    size_t effect;

    /// The implications which define the meaning of the effect.
    std::vector<EffectImplication> implications;
};

struct Handler {
    Handler(size_t effect, BuiltinHandler h):
        effect(effect), implementation(h) {}
    Handler(UserHandler h):
        effect(h.effect), implementation(h) {}

    /// A number which uniquely identifies the effect type that this handler
    /// handles.
    size_t effect;

    /// The underlying implementation for this handler.
    TaggedUnion<
        BuiltinHandler,
        UserHandler
    > implementation;
};

bool operator==(const UserHandler &, const UserHandler &);
bool operator!=(const UserHandler &, const UserHandler &);
std::ostream& operator<<(std::ostream &out, const UserHandler &h);

struct Predicate {
    Predicate(
        std::vector<Implication> implications,
        std::vector<UserHandler> handlers
    ): implications(implications), handlers(handlers) {}

    Predicate operator=(Predicate other) {
        swap(implications, other.implications);
        return *this;
    }

    std::vector<Implication> implications;
    std::vector<UserHandler> handlers;
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
        std::vector<std::string> predicateNameTable = {},
        Config config = Config()
    ): predicates(ps), entryPoint(main), predicateNameTable(predicateNameTable),
        config(config) {}
    
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

    /// Writes a representation of the predicate reference for debugging.
    std::string asDebugString(const PredicateReference &pr) const;

    /// The program's configuration parameters.
    const Config config;

protected:
    /// A collection of the predicates defined in the program. Predicates
    /// refer to each other through their indices in this vector.
    std::vector<Predicate> predicates;

    /// The predicate which represents whether the program accepts or
    /// rejects, if there is one, i.e. `main`.
    ///
    /// There may be no such value if the program doesn't define `main`.
    /// Such a program always rejects.
    Optional<PredicateReference> entryPoint;

    const std::vector<std::string> predicateNameTable;
};

} // namespace interpreter

#endif // INTERPRETER_H
