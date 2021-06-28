#include <algorithm>
#include <limits>

#include "SemAna/TypedAST.h"
#include "SemAna/VariableAnalysis.h"
#include "Interpreter/ASTLower.h"

using namespace TypedAST;

/// Traverses the AST and lowers it to interpreter primitives using
/// the visitor pattern.
class ASTLowerer {
    /// The implication enclosing the current AST node being analyzed, if there is one.
    Optional<Implication> enclosingImplication;

public:
    ASTLowerer(const AST &ast): ast(ast) {}

    interpreter::VariableRef visit(const AnonymousVariable &av) {
        bool isTypeInhabited = ast.resolveTypeRef(av.type).constructors.size() > 0;
        return interpreter::VariableRef(isTypeInhabited);
    }

    interpreter::VariableRef visit(const Variable &v) {
        std::unique_ptr<Implication> impl;
        if(enclosingImplication.unwrapGuard(impl)) {
            assert(false && "implication not set!");
        }
        size_t index = getVariableIndex(*impl, v);
        bool isTypeInhabited = ast.resolveTypeRef(v.type).constructors.size() > 0;
        return interpreter::VariableRef(
            index,
            v.isDefinition,
            v.isExistential,
            isTypeInhabited);
    }

    /// Note: To support type inference, this requires the additional
    /// context information of the type. 
    interpreter::ConstructorRef visit(const ConstructorRef &cr, const TypeRef &tr) {
        size_t index = getConstructorIndex(tr, cr);
        const Constructor &ctor = ast.resolveConstructorRef(tr, cr);

        std::vector<interpreter::Value> arguments;
        for(int i=0; i<cr.arguments.size(); ++i) {
            arguments.push_back(visit(cr.arguments[i], ctor.parameters[i]));
        }

        return interpreter::ConstructorRef(index, arguments);
    }

    interpreter::String visit(const StringLiteral &str) {
        return interpreter::String(str.value);
    }

    interpreter::Value visit(const Value &val, const TypeRef &tr) {
        return val.match<interpreter::Value>(
            [&](AnonymousVariable av) { return interpreter::Value(visit(av)); },
            [&](Variable v) { return interpreter::Value(visit(v)); },
            [&](ConstructorRef cr) { return interpreter::Value(visit(cr, tr)); },
            [&](StringLiteral str) { return interpreter::Value(visit(str)); }
        );
    }

    interpreter::TruthValue visit(const TruthLiteral &tl) {
        return interpreter::TruthValue(tl.value);
    }

    interpreter::PredicateReference visit(const PredicateRef &pr) {
        size_t predicateIndex = getPredicateIndex(pr.name);

        std::vector<interpreter::Value> arguments;
        const Predicate &p = ast.resolvePredicateRef(pr);
        for(int i=0; i<p.declaration.parameters.size(); ++i) {
            auto loweredCtorRef = visit(pr.arguments[i], p.declaration.parameters[i]);
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
        [&](TruthLiteral tl) { return interpreter::Expression(visit(tl)); },
        [&](PredicateRef pr) { return interpreter::Expression(visit(pr)); },
        [&](Conjunction conj) { return interpreter::Expression(visit(conj)); }
        );
    }

    interpreter::Implication visit(const Implication &impl) {
        enclosingImplication = impl;
        auto head = visit(impl.head);
        auto body = visit(impl.body);
        enclosingImplication = Optional<Implication>();

        size_t variableCount = getVariables(ast, impl).size();
        return interpreter::Implication(head, body, variableCount);
    }

    interpreter::Predicate visit(const Predicate &p) {
        std::vector<interpreter::Implication> implications;
        implications.reserve(p.implications.size());
        for(const Implication &impl : p.implications) {
            implications.push_back(visit(impl));
        }
        return interpreter::Predicate(implications);
    }

    void visit(const PredicateDecl &pd) { assert(false && "not implemented"); };

    void visit(const TypeDecl &td) { assert(false && "not implemented"); };

    void visit(const TypeRef &tr) { assert(false && "not implemented"); };

    void visit(const Constructor &ctor) { assert(false && "not implemented"); };

    void visit(const ConstructorRef &cr) { assert(false && "not implemented"); };

    void visit(const Value &val) { assert(false && "not implemented"); };

    void visit(const Type &t) { assert(false && "not implemented"); };

private:
    size_t getPredicateIndex(const Name<Predicate> &pn) {
        // TODO: it should be possible to do this in logarithmic time,
        // but the current implementation is linear.
        auto x = std::find_if(
            ast.predicates.begin(),
            ast.predicates.end(),
            [&](const Predicate &p) { return p.declaration.name == pn; });

        assert(x != ast.predicates.end());
        return x - ast.predicates.begin();
    }

    size_t getConstructorIndex(const TypeRef &tr, const ConstructorRef &cr) {
        auto type = std::find_if(
            ast.types.begin(),
            ast.types.end(),
            [&](const Type &type) { return type.declaration.name == tr; });
        
        assert(type != ast.types.end());
        
        auto ctor = std::find_if(
            type->constructors.begin(),
            type->constructors.end(),
            [&](const Constructor &ctor) { return ctor.name == cr.name; });

        assert(ctor != type->constructors.end());
        return ctor - type->constructors.begin();
    }

    /// Computes the implication's variable list and returns the variable's
    /// index within it.
    size_t getVariableIndex(const Implication &impl, const Variable &v) {
        Scope scope = getVariables(ast, impl);
        size_t index = 0;
        auto vIter = scope.find(v.name);
        for(; vIter != scope.begin(); --vIter) ++index;
        return index;
    }

    const AST &ast;
};

// TODO: new assert for TypedAST
//static_assert(has_all_visitors<ASTLowerer>(), "ASTLowerer missing visitor(s).");

interpreter::Program lower(const AST &ast) {
    ASTLowerer lowerer(ast);
    
    std::vector<interpreter::Predicate> loweredPredicates;
    loweredPredicates.reserve(ast.predicates.size());

    Optional<interpreter::PredicateReference> main;

    size_t i = 0;
    for(const auto &p : ast.predicates) {
        interpreter::Predicate lowered = lowerer.visit(p);
        loweredPredicates.push_back(lowered);

        if(p.declaration.name == Name<Predicate>("main"))
            // If main did take arguments, this would be the place to
            // pass them to the program!
            main = interpreter::PredicateReference(i, {});
        ++i;
    }

    return interpreter::Program(loweredPredicates, main);
}
