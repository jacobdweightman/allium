#ifndef SEMANA_PRED_RECURSION_ANALYSIS_H
#define SEMANA_PRED_RECURSION_ANALYSIS_H

#include <map>
#include <set>

#include "SemAna/TypedAST.h"

namespace TypedAST {

/// A digraph which has a vertex for each predicate in a program, and there is
/// a directed edge from p to q if q occurs in the body of one of p's
/// implications.
class PredDependenceGraph {
    std::map<Name<Predicate>, std::set<Name<Predicate>>> adjacencyList;

    bool dependsOnHelper(
        const Name<Predicate> &first,
        const Name<Predicate> &second,
        std::set<Name<Predicate>> &visited
    ) const;

public:
    PredDependenceGraph(const AST &ast);

    /// True iff `name` is the name of a predicate which may occur in a
    /// sub-proof of itself.
    bool isRecursive(const Name<Predicate> &name) const;

    /// True iff `second` may occur in a sub-proof of `first`.
    bool dependsOn(
        const Name<Predicate> &first,
        const Name<Predicate> &second
    ) const;
};

inline void forAllPredRefs(
    const Expression &expr,
    std::function<void(const PredicateRef &)> f
) {
    expr.switchOver(
    [](const TruthLiteral &) {},
    [&](const PredicateRef &pr) { f(pr); },
    [](const EffectCtorRef &) {},
    [&](const Conjunction &conj) {
        forAllPredRefs(conj.getLeft(), f);
        forAllPredRefs(conj.getRight(), f);
    });
}

} // namespace TypedAST

#endif // SEMANA_PRED_RECURSION_ANALYSIS_H
