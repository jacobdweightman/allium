#ifndef SEMANA_PREDICATE_H
#define SEMANA_PREDICATE_H

#include <vector>
#include <map>

#include "Parser/AST.h"
#include "SemAna/StaticError.h"

/// Represents a complete program for the purpose of semantic analysis.
class Program {
public:
    Program(
        std::vector<Type> ts,
        std::vector<Predicate> ps,
        ErrorEmitter &error
    ): error(error) {
        for(const Type &t : ts) {
            assert(types.count(t.declaration.name) == 0);
            types.emplace(t.declaration.name, t);
        }

        for(const Predicate &p : ps) {
            assert(predicates.count(p.name.name) == 0);
            predicates.emplace(p.name.name, p);
        }
    }

    /// Performs all semantic analyses and emits all diagnostics.
    void checkAll();

    const Type &resolveTypeRef(const TypeRef &tr) const {
        const auto x = types.find(tr.name);
        assert(x != types.end());
        return x->second;
    }

    const Constructor &resolveConstructorRef(const TypeRef &tr, const ConstructorRef &cr) const {
        const Type &type = resolveTypeRef(tr);

        auto ctor = type.constructors.begin();
        for(; ctor!=type.constructors.end(); ++ctor) {
            if(ctor->name == cr.name) break;
        }
        assert(ctor != type.constructors.end());
        return *ctor;
    }

    const Predicate &resolvePredicateRef(const PredicateRef &pr) const {
        const auto x = predicates.find(pr.name);
        assert(x != predicates.end());
        return x->second;
    }

    /// a map to access predicate definitions from their names.
    std::map<Name<Predicate>, Predicate> predicates;

    /// a map to access type definitions from their names.
    std::map<Name<Type>, Type> types;

    /// Represents the variables and their types defined in a scope.
    typedef std::map<Name<Variable>, Type> Scope;

    struct ImplicationCMP {
        bool operator()(const Implication &a, const Implication &b) const {
            // sort implications by the order they appear in source. This will
            // need to be reconsidered for multi-file support.
            return a.lhs.location < b.lhs.location;
        }
    };

    /// access variable types by name and the implication where they occur.
    /// These are built up dynamically while analyzing the program. A
    /// function of type Implication -> Name<Variable> -> TypeRef.
    mutable std::map<Implication, Scope, ImplicationCMP> scopes;

private:
    ErrorEmitter &error;
};

#endif // SEMANA_PREDICATE_H
