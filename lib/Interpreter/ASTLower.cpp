#include <algorithm>

#include "Interpreter/ASTLower.h"

/// Traverses the AST and lowers it to interpreter primitives using
/// the visitor pattern.
class ASTLowerer {
public:
    ASTLowerer(const Program &semanaProgram): semanaProgram(semanaProgram) {}

    interpreter::ConstructorRef visit(const AnonymousVariable &av) {
        return interpreter::ConstructorRef(-1, {});
    }

    /// Note: To support type inference, this requires the additional
    /// context information of the type. 
    interpreter::ConstructorRef visit(const ConstructorRef &cr, const TypeRef &tr) {
        size_t index = getConstructorIndex(tr, cr);
        const Constructor &ctor = semanaProgram.resolveConstructorRef(tr, cr);

        std::vector<interpreter::ConstructorRef> arguments;
        for(int i=0; i<cr.arguments.size(); ++i) {
            arguments.push_back(visit(cr.arguments[i], ctor.parameters[i]));
        }

        return interpreter::ConstructorRef(index, arguments);
    }

    interpreter::ConstructorRef visit(const Value &val, const TypeRef &tr) {
        return val.match<interpreter::ConstructorRef>(
            [&](AnonymousVariable av) { return visit(av); },
            [&](ConstructorRef cr) { return visit(cr, tr); }
        );
    }

    interpreter::TruthValue visit(const TruthLiteral &tl) {
        return interpreter::TruthValue(tl.value);
    }

    interpreter::PredicateReference visit(const PredicateRef &pr) {
        size_t predicateIndex = getPredicateIndex(pr.name);

        std::vector<interpreter::ConstructorRef> arguments;
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
        return interpreter::Implication(visit(impl.lhs), visit(impl.rhs));
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
            if(ctor.name == cr.name) break;
            ++i;
        }
        return i;
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
