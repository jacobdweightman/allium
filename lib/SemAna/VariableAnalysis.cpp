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
            const Type &type = ast.resolveTypeRef(v.type);
            scope.insert({ v.name, type });
        },
        [&](ConstructorRef cr) {
            for(const auto &arg : cr.arguments) getVariables(arg);
        });
    }

    void getVariables(const PredicateRef &pr) {
        for(const auto &arg : pr.arguments) getVariables(arg);
    }

    void getVariables(const Conjunction &conj) {
        getVariables(conj.getLeft());
        getVariables(conj.getRight());
    }

    void getVariables(const Expression &expr) {
        expr.switchOver(
        [](TruthLiteral) {},
        [&](PredicateRef pr) { getVariables(pr); },
        [&](Conjunction conj) { getVariables(conj); }
        );
    }

public:
    VariableAnalysis(const AST &ast): ast(ast) {}

    Scope getVariables(const Implication impl) {
        scope = Scope();
        getVariables(impl.head);
        getVariables(impl.body);
        return scope;
    }
};

/// Returns the variables and their types which are defined in the
/// lexical scope of the given implication.
Scope getVariables(const AST &ast, const Implication impl) {
    return VariableAnalysis(ast).getVariables(impl);
}

}
