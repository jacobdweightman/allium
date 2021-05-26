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
        });
    }

    void getVariables_(const PredicateRef &pr) {
        for(const auto &arg : pr.arguments) getVariables(arg);
    }

    void getVariables(const Conjunction &conj) {
        getVariables(conj.getLeft());
        getVariables(conj.getRight());
    }

public:
    VariableAnalysis(const AST &ast): ast(ast) {}

    Scope getVariables(const PredicateRef &pr) {
        getVariables_(pr);
        return scope;
    }

    Scope getVariables(const Expression &expr) {
        expr.switchOver(
        [](TruthLiteral) {},
        [&](PredicateRef pr) { getVariables_(pr); },
        [&](Conjunction conj) { getVariables(conj); }
        );
        return scope;
    }
};

/// Returns the variables and their types which are defined inside of the given
/// predicate reference.
///
/// This is primarily used to find possibly universally quantified variables
/// from an implication by traversing its head.
Scope getVariables(const AST &ast, const PredicateRef pr) {
    return VariableAnalysis(ast).getVariables(pr);
}

/// Returns the variables and their types which are defined inside of the given
/// expression.
///
/// This is primarily used to get the existentially quantified variables from an
/// implication by traversing its body.
Scope getVariables(const AST &ast, const Expression expr) {
    return VariableAnalysis(ast).getVariables(expr);
}

}
