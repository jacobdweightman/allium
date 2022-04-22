#include <algorithm>
#include <limits>

#include "Interpreter/ASTLower.h"
#include "SemAna/Builtins.h"
#include "SemAna/InhabitableAnalysis.h"
#include "SemAna/TypedAST.h"
#include "SemAna/VariableAnalysis.h"
#include "Utils/VectorUtils.h"

using namespace TypedAST;

/// Traverses the AST and lowers it to interpreter primitives using
/// the visitor pattern.
class ASTLowerer {
    /// The implication enclosing the current AST node being analyzed, if there is one.
    Optional<Implication> enclosingImplication;

public:
    ASTLowerer(
        const AST &ast
    ): ast(ast), inhabitableTypes(getInhabitableTypes(ast.types)) {}

    interpreter::MatcherVariable visit(const AnonymousVariable &av) {
        bool isTypeInhabited = inhabitableTypes.contains(av.type);
        return interpreter::MatcherVariable(
            interpreter::MatcherVariable::anonymousIndex,
            isTypeInhabited);
    }

    interpreter::MatcherVariable visit(const Variable &v) {
        std::unique_ptr<Implication> impl;
        if(enclosingImplication.unwrapGuard(impl)) {
            assert(false && "implication not set!");
        }
        size_t index = getVariableIndex(*impl, v);
        bool isTypeInhabited = inhabitableTypes.contains(v.type);
        return interpreter::MatcherVariable(index, isTypeInhabited);
    }

    /// Note: To support type inference, this requires the additional
    /// context information of the type. 
    interpreter::MatcherCtorRef visit(const ConstructorRef &cr, const Name<Type> &tr) {
        size_t index = getConstructorIndex(tr, cr);
        const Constructor &ctor = ast.resolveConstructorRef(tr, cr);

        std::vector<interpreter::MatcherValue> arguments;
        for(int i=0; i<cr.arguments.size(); ++i) {
            arguments.push_back(visit(cr.arguments[i], ctor.parameters[i].type));
        }

        return interpreter::MatcherCtorRef(index, arguments);
    }

    interpreter::String visit(const StringLiteral &str) {
        return interpreter::String(str.value);
    }

    interpreter::Int visit(const IntegerLiteral &i) {
        return interpreter::Int(i.value);
    }

    interpreter::MatcherValue visit(const Value &val, const Name<Type> &tr) {
        return val.match<interpreter::MatcherValue>(
            [&](AnonymousVariable av) { return interpreter::MatcherValue(visit(av)); },
            [&](Variable v) { return interpreter::MatcherValue(visit(v)); },
            [&](ConstructorRef cr) { return interpreter::MatcherValue(visit(cr, tr)); },
            [&](StringLiteral str) { return interpreter::MatcherValue(visit(str)); },
            [&](IntegerLiteral i) { return interpreter::MatcherValue(visit(i)); }
        );
    }

    interpreter::TruthValue visit(const TruthLiteral &tl) {
        return interpreter::TruthValue(tl.value);
    }

    interpreter::PredicateReference visit(const PredicateRef &pr) {
        size_t predicateIndex = getPredicateIndex(pr.name);

        std::vector<interpreter::MatcherValue> arguments;
        const Predicate &p = ast.resolvePredicateRef(pr);
        for(int i=0; i<p.declaration.parameters.size(); ++i) {
            auto loweredCtorRef = visit(pr.arguments[i], p.declaration.parameters[i].type);
            arguments.push_back(loweredCtorRef);
        }
        
        return interpreter::PredicateReference(predicateIndex, arguments);
    }

    interpreter::EffectCtorRef visit(const EffectCtorRef &ecr) {
        const EffectCtor &eCtor = ast.resolveEffectCtorRef(ecr);
        auto [effectIndex, eCtorIndex] = getEffectIndices(ecr);

        std::vector<interpreter::MatcherValue> arguments;
        for(int i=0; i<eCtor.parameters.size(); ++i) {
            auto loweredCtorRef = visit(ecr.arguments[i], eCtor.parameters[i].type);
            arguments.push_back(loweredCtorRef);
        }

        return interpreter::EffectCtorRef(effectIndex, eCtorIndex, arguments);
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
        [&](EffectCtorRef ecr) { return interpreter::Expression(visit(ecr)); },
        [&](Conjunction conj) { return interpreter::Expression(visit(conj)); }
        );
    }

    interpreter::Implication visit(const Implication &impl) {
        enclosingImplication = impl;
        auto head = visit(impl.head);
        auto body = visit(impl.body);
        enclosingImplication = Optional<Implication>();

        size_t variableCount = getVariables(impl).size();
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

    void visit(const CtorParameter &cp) { assert(false && "not implemented"); };

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

    size_t getConstructorIndex(const Name<Type> &tr, const ConstructorRef &cr) {
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
        auto variables = getVariables(impl);
        auto var = std::find(variables.begin(), variables.end(), v.name);
        return var - variables.begin();
    }

    std::pair<size_t, size_t> getEffectIndices(const EffectCtorRef &ecr) {
        auto effect = std::find_if(
            ast.effects.begin(),
            ast.effects.end(),
            [&](const Effect &e) { return ecr.effectName == e.declaration.name; });

        if(effect != ast.effects.end()) {
            auto eCtor = std::find_if(
                effect->constructors.begin(),
                effect->constructors.end(),
                [&](const EffectCtor &eCtor) { return ecr.ctorName == eCtor.name; });
            
            assert(eCtor != effect->constructors.end());

            return {
                effect - ast.effects.begin() + TypedAST::builtinEffects.size(),
                eCtor - effect->constructors.begin()
            };
        }

        effect = std::find_if(
            builtinEffects.begin(),
            builtinEffects.end(),
            [&](const Effect &e) { return e.declaration.name == ecr.effectName; });
        
        // The effect must either be in the AST or a builtin effect.
        assert(effect != builtinEffects.end());
        
        auto eCtor = std::find_if(
            effect->constructors.begin(),
            effect->constructors.end(),
            [&](const EffectCtor &eCtor) { return ecr.ctorName == eCtor.name; });
        
        assert(eCtor != effect->constructors.end());
        return {
            effect - builtinEffects.begin(),
            eCtor - effect->constructors.begin()
        };
    }

    const AST &ast;
    const std::set<Name<Type>> inhabitableTypes;
};

// TODO: new assert for TypedAST
//static_assert(has_all_visitors<ASTLowerer>(), "ASTLowerer missing visitor(s).");

interpreter::Program lower(const AST &ast, interpreter::Config config) {
    ASTLowerer lowerer(ast);
    
    std::vector<interpreter::Predicate> loweredPredicates;
    loweredPredicates.reserve(ast.predicates.size());

    std::vector<std::string> predicateNameTable;
    predicateNameTable.reserve(ast.predicates.size());

    Optional<interpreter::PredicateReference> main;

    size_t i = 0;
    for(const auto &p : ast.predicates) {
        interpreter::Predicate lowered = lowerer.visit(p);
        loweredPredicates.push_back(lowered);

        predicateNameTable.push_back(p.declaration.name.string());

        if(p.declaration.name == Name<Predicate>("main"))
            // If main did take arguments, this would be the place to
            // pass them to the program!
            main = interpreter::PredicateReference(i, {});
        ++i;
    }

    return interpreter::Program(
        loweredPredicates,
        main,
        predicateNameTable,
        config);
}
