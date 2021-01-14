#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <algorithm>
#include <assert.h>
#include <limits>
#include <vector>

#include "../values/TaggedUnion.h"

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
struct Conjunction;

typedef TaggedUnion<
    TruthValue,
    PredicateReference,
    Conjunction
> Expression;

struct ProgramNode {};

struct ConstructorRef: public ProgramNode {
    ConstructorRef(size_t index, std::vector<ConstructorRef> arguments): 
        index(index), arguments(arguments) {}

    bool matches(const ConstructorRef &other) const;

    static const size_t anonymousVariable = std::numeric_limits<size_t>::max();

    size_t index;
    std::vector<ConstructorRef> arguments;
};

struct TruthValue: public ProgramNode {
    TruthValue(bool value): value(value) {}

    bool value;
};

struct PredicateReference: public ProgramNode {
    PredicateReference(size_t index, std::vector<ConstructorRef> arguments): 
        index(index), arguments(arguments) {}
    
    bool matches(const PredicateReference &other) const;

    /// The index into the program's predicate list.
    size_t index;

    std::vector<ConstructorRef> arguments;
};

struct Conjunction: public ProgramNode {
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

    Expression &getLeft() { return *left; }
    Expression &getRight() { return *right; }
private:
    std::unique_ptr<Expression> left, right;
};

struct Implication: public ProgramNode {
    Implication(PredicateReference head, Expression body):
        head(head), body(body) {}

    PredicateReference head;
    Expression body;
};

struct Predicate: public ProgramNode {
    Predicate(std::vector<Implication> implications):
        implications(implications) {}

    Predicate operator=(Predicate other) {
        swap(implications, other.implications);
        return *this;
    }

    std::vector<Implication> implications;
};

class Program: public ProgramNode {
public:
    Program(std::vector<Predicate> ps, Optional<PredicateReference> main):
        predicates(ps), entryPoint(main) {}

    bool prove(const Expression&);

    const Optional<PredicateReference> &getEntryPoint() {
        return entryPoint;
    }

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
};

} // namespace interpreter

#endif // INTERPRETER_H
