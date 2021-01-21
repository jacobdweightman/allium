#include <algorithm>
#include <limits>

#include "Interpreter/ASTLower.h"

/// Traverses the AST and lowers it to interpreter primitives using
/// the visitor pattern.
class ASTLowerer {
    /// The implication enclosing the current AST node being analyzed, if there is one.
    Optional<Implication> enclosingImplication;

public:
    ASTLowerer(const Program &semanaProgram): semanaProgram(semanaProgram) {}

    interpreter::VariableRef visit(const AnonymousVariable &av) {
        return interpreter::VariableRef();
    }

    interpreter::VariableRef visit(const Variable &v) {
        Implication impl;
        if(enclosingImplication.unwrapGuard(impl)) {
            assert(false && "implication not set!");
        }
        size_t index = getVariableIndex(impl, v);
        return interpreter::VariableRef(index, v.isDefinition);
    }

    /// Note: To support type inference, this requires the additional
    /// context information of the type. 
    interpreter::ConstructorRef visit(const ConstructorRef &cr, const TypeRef &tr) {
        size_t index = getConstructorIndex(tr, cr);
        const Constructor &ctor = semanaProgram.resolveConstructorRef(tr, cr);

        std::vector<interpreter::Value> arguments;
        for(int i=0; i<cr.arguments.size(); ++i) {
            arguments.push_back(visit(cr.arguments[i], ctor.parameters[i]));
        }

        return interpreter::ConstructorRef(index, arguments);
    }

    interpreter::Value visit(const Value &val, const TypeRef &tr) {
        return val.match<interpreter::Value>(
            [&](AnonymousVariable av) { return visit(av); },
            [&](Variable v) { return visit(v); },
            [&](ConstructorRef cr) -> interpreter::Value {
                // disambiguate constructors without arguments from variables
                if(getConstructorIndex(tr, cr) == std::numeric_limits<size_t>::max()) {
                    Variable v(cr.name.string(), false, cr.location);
                    return visit(v);
                } else {
                    return visit(cr, tr);
                }
            }
        );
    }

    interpreter::TruthValue visit(const TruthLiteral &tl) {
        return interpreter::TruthValue(tl.value);
    }

    interpreter::PredicateReference visit(const PredicateRef &pr) {
        size_t predicateIndex = getPredicateIndex(pr.name);

        std::vector<interpreter::Value> arguments;
        const Predicate &p = semanaProgram.resolvePredicateRef(pr);
        for(int i=0; i<p.name.parameters.size(); ++i) {
            auto loweredCtorRef = visit(pr.arguments[i], p.name.parameters[i]);
            arguments.push_back(loweredCtorRef);
        }
        
        return interpreter::PredicateReference(predicateIndex, arguments);
    }

    interpreter::Conjunction visit(const Conjunction &conj) {
        return interpreter::Conjunction(
            visit(conj.getLeft()),
            visit(conj.getRight())
        );
    }

    interpreter::Expression visit(const Expression &expr) {
        return expr.match<interpreter::Expression>(
        [&](TruthLiteral tl) { return visit(tl); },
        [&](PredicateRef pr) { return visit(pr); },
        [&](Conjunction conj) { return visit(conj); }
        );
    }

    interpreter::Implication visit(const Implication &impl) {
        enclosingImplication = impl;
        auto head = visit(impl.lhs);
        auto body = visit(impl.rhs);
        enclosingImplication = Optional<Implication>();

        size_t numVariables = semanaProgram.scopes[impl].size();
        return interpreter::Implication(head, body, numVariables);
    }

    interpreter::Predicate visit(const Predicate &p) {
        std::vector<interpreter::Implication> implications;
        implications.reserve(p.implications.size());
        for(const Implication &impl : p.implications) {
            implications.push_back(visit(impl));
        }
        return interpreter::Predicate(implications);
    }

private:
    size_t getPredicateIndex(const Name<Predicate> &pn) {
        // TODO: it should be possible to do this in logarithmic time,
        // but the current implementation is linear.
        auto x = semanaProgram.predicates.find(pn);
        assert(x != semanaProgram.predicates.end());
        size_t i = 0;
        for(; x != semanaProgram.predicates.begin(); --x) ++i;
        return i;
    }

    size_t getConstructorIndex(const TypeRef &tr, const ConstructorRef &cr) {
        const auto typeIter = semanaProgram.types.find(tr.name);
        assert(typeIter != semanaProgram.types.end());
        const auto &type = typeIter->second;

        size_t i = 0;
        for(const Constructor &ctor : type.constructors) {
            if(ctor.name == cr.name) return i;
            ++i;
        }
        // the "constructor reference" is actually a variable.
        return std::numeric_limits<size_t>::max(); 
    }

    /// Computes the implication's variable list and returns the variable's
    /// index within it.
    size_t getVariableIndex(const Implication &impl, const Variable &v) {
        auto scopeIter = semanaProgram.scopes.find(impl);
        assert(scopeIter != semanaProgram.scopes.end());
        auto scope = scopeIter->second;

        size_t index = 0;
        auto vIter = scope.find(v.name);
        for(; vIter != scope.begin(); --vIter) ++index;
        return index;
    }

    const Program &semanaProgram;
};

interpreter::Program lower(const Program &semanaProgram) {
    ASTLowerer lowerer(semanaProgram);
    
    std::vector<interpreter::Predicate> loweredPredicates;
    loweredPredicates.reserve(semanaProgram.predicates.size());

    Optional<interpreter::PredicateReference> main;

    size_t i = 0;
    for(const auto &p : semanaProgram.predicates) {
        interpreter::Predicate lowered = lowerer.visit(p.second);
        loweredPredicates.push_back(lowered);

        if(p.second.name.name == Name<Predicate>("main"))
            // If main did take arguments, this would be the place to
            // pass them to the program!
            main = Optional(interpreter::PredicateReference(i, {}));
        ++i;
    }

    return interpreter::Program(loweredPredicates, main);
}
