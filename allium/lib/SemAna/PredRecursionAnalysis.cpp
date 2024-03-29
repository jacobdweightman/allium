
#include <functional>

#include "SemAna/PredRecursionAnalysis.h"

namespace TypedAST {

PredDependenceGraph::PredDependenceGraph(const AST &ast) {
    for(const auto &p : ast.predicates) {
        adjacencyList[p.declaration.name] = {};
        for(const auto &impl : p.implications) {
            forAllPredRefs(impl.body, [&](const PredicateRef &pr) {
                if(ast.resolvePredicateRef(pr).is_a<const UserPredicate*>()) {
                    adjacencyList[p.declaration.name].insert(pr.name);
                }
            });
        }
    }
}

/// True iff `name` is the name of a predicate which may occur in a
/// sub-proof of itself.
bool PredDependenceGraph::isRecursive(const Name<Predicate> &name) const {
    return dependsOn(name, name);
}

bool PredDependenceGraph::dependsOnHelper(
    const Name<Predicate> &first,
    const Name<Predicate> &second,
    std::set<Name<Predicate>> &visited
) const {
    // There's no need to track recursion for builtin predicates, since this is
    // an implementation detail. Any place where this "would be" used needs to
    // get this information from somewhere else; for example, ground analysis
    // uses explicitly tabulated groundness information for each builtin.
    const auto callees = adjacencyList.find(first);
    if(callees == adjacencyList.end())
        return false;

    if(adjacencyList.at(first).contains(second))
        return true;

    visited.insert(first);

    for(const auto &x : adjacencyList.at(first)) {
        if(!visited.contains(x) && dependsOnHelper(x, second, visited))
            return true;
    }

    return false;
}

/// True iff `second` may occur in a sub-proof of `first`.
bool PredDependenceGraph::dependsOn(
    const Name<Predicate> &first,
    const Name<Predicate> &second
) const {
    std::set<Name<Predicate>> visited;
    return dependsOnHelper(first, second, visited);

}

} // namespace TypedAST

