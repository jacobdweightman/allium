#include "SemAna/VariableAnalysis.h"

namespace TypedAST {

class VariableAnalysis {
private:
    const AST &ast;
    mutable Scope scope;

    void getVariables(const Value &value) {
        value.switchOver(
        [](AnonymousVariable av) {},
        [&](Variable v) {
            if(v.isDefinition) {
                const Type &type = ast.resolveTypeRef(v.type);
                scope.insert({ v.name, VariableInfo(type, v.isExistential) });
            }
        },
        [&](ConstructorRef cr) {
            for(const auto &arg : cr.arguments) getVariables(arg);
        },
        [&](StringLiteral) {});
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
    VariableAnalysis(const AST &ast): ast(ast) {}

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

    Scope getScope() {
        return scope;
    }
};

Scope getVariables(const AST &ast, const Implication &impl) {
    VariableAnalysis va(ast);
    va.getVariables(impl.head);
    va.getVariables(impl.body);
    return va.getScope();
}

}
