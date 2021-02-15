#include "Interpreter/program.h"

namespace interpreter {

struct WitnessProducer {
    virtual ~WitnessProducer() = 0;

    virtual bool nextWitness() = 0;

    Expression instantiate(const Expression &expr) const;

    /// Any variables defined in the head of an implication are universally
    /// quantified. For example:
    ///     `p(let x) <- q(x);` is equivalent to `∀x, (q(x) -> p(x)).
    /// Given some value `y` of the appropriate type, proving `p(y)` using this
    /// implication replaces all occurences of `x` with `y`.
    std::vector<ConstructorRef> universalVariables;
};

std::unique_ptr<WitnessProducer> createWitnessProducer(
    const interpreter::Program &program,
    interpreter::Expression expr
);

struct TruthValueWitnessProducer : public WitnessProducer {
    TruthValueWitnessProducer(interpreter::TruthValue tv):
        WitnessProducer(), nextValue(tv.value) {}

    ~TruthValueWitnessProducer() = default;

    bool nextWitness() override;

private:
    bool nextValue;
};

struct PredicateRefWitnessProducer : public WitnessProducer {
    PredicateRefWitnessProducer(
        const interpreter::Program &program,
        interpreter::PredicateReference pr
    ): WitnessProducer(), pr(pr), program(program),
        predicate(&program.getPredicate(pr.index)),
        currentImpl(predicate->implications.begin()) {
            std::cout << "prove: " << pr << "\n";
            setUpNextMatchingImplication();
    }

    PredicateRefWitnessProducer(PredicateRefWitnessProducer &&other) = default;

    ~PredicateRefWitnessProducer() override;

    bool nextWitness() override;

    // Support pattern matching against the heads of implications which could be
    // used to prove a predicate. Pattern matching will assign values to as many
    // variables in the head of the implication as possible, and the rest become
    // existentially quantified variables which become part of the
    // WitnessProducer's witnesses.

    bool match(const PredicateReference &other);

    bool match(const VariableRef &vl, const VariableRef &vr);
    bool match(const VariableRef &vr, const ConstructorRef &cr);
    bool match(const ConstructorRef &cl, const ConstructorRef &cr);
    bool match(const Value &left, const Value &right);

private:
    /// Sets up the witness producer's state for working with the next
    /// implication.
    ///
    /// If it were possible to write these classes as generators/resumable
    /// functions, then this would be handled by the control flow.
    void setUpNextMatchingImplication() {
        while(currentImpl != predicate->implications.end()) {
            std::cout << "next implication\n";
            universalVariables.clear();
            universalVariables.resize(currentImpl->headVariableCount);

            if(match(currentImpl->head)) {
                currentWitnessProducer = createWitnessProducer(program, instantiate(currentImpl->body));
                break;
            } else {
                ++currentImpl;
            }
        }
    }

    const interpreter::PredicateReference pr;
    const interpreter::Program &program;
    const interpreter::Predicate *predicate;
    std::vector<interpreter::Implication>::const_iterator currentImpl;
    std::unique_ptr<WitnessProducer> currentWitnessProducer;
};

struct ConjunctionWitnessProducer : public WitnessProducer {
    ConjunctionWitnessProducer(
        const interpreter::Program &program,
        interpreter::Conjunction conj
    ): WitnessProducer(), program(program), conj(conj),
        leftWitnessProducer(createWitnessProducer(program, conj.getLeft())),
        rightWitnessProducer(createWitnessProducer(program, conj.getRight()))
    {
        foundLeftWitness = leftWitnessProducer->nextWitness();
    }

    ConjunctionWitnessProducer(ConjunctionWitnessProducer &&other) = default;

    ~ConjunctionWitnessProducer() = default;

    bool nextWitness() override;

private:
    const interpreter::Program &program;
    interpreter::Conjunction conj;
    std::unique_ptr<WitnessProducer> leftWitnessProducer;
    std::unique_ptr<WitnessProducer> rightWitnessProducer;

    /// Keep track of whether or not the left witness producer last succeeded.
    bool foundLeftWitness;
};

} // namespace interpreter
