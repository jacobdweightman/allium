#include "SemAna/VariableAnalysis.h"

namespace TypedAST {

class VariableAnalysis {
private:
    std::vector<Name<Variable>> variables;

    void getVariables(const Value &value) {
        value.switchOver(
        [](AnonymousVariable av) {},
        [&](Variable v) {
            if(v.isDefinition) {
                variables.push_back(v.name);
            }
        },
        [&](ConstructorRef cr) {
            for(const auto &arg : cr.arguments) getVariables(arg);
        },
        [&](StringLiteral) {},
        [&](IntegerLiteral) {});
    }

    void getVariables_(const PredicateRef &pr) {
        for(const auto &arg : pr.arguments) getVariables(arg);
    }

    void getVariables(const EffectCtorRef &ecr) {
        for(const auto &arg : ecr.arguments) getVariables(arg);
    }

    void getVariables(const Conjunction &conj) {
        getVariables(conj.getLeft());
        getVariables(conj.getRight());
    }

public:
    void getVariables(const PredicateRef &pr) {
        getVariables_(pr);
    }

    void getVariables(const Expression &expr) {
        expr.switchOver(
        [](TruthLiteral) {},
        [&](PredicateRef pr) { getVariables_(pr); },
        [&](EffectCtorRef ecr) { getVariables(ecr); },
        [&](Conjunction conj) { getVariables(conj); }
        );
    }

    std::vector<Name<Variable>> getVariables() {
        return variables;
    }
};

std::vector<Name<Variable>> getVariables(const Implication &impl) {
    VariableAnalysis va;
    va.getVariables(impl.head);
    va.getVariables(impl.body);
    return va.getVariables();
}

}
