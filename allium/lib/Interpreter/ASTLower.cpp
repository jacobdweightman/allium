#include <algorithm>
#include <limits>

#include "Interpreter/ASTLower.h"
#include "Interpreter/BuiltinPredicates.h"
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
        size_t index = getTypeConstructorIndex(tr, cr);
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

    interpreter::Continuation visit(const Continuation &k) {
        return interpreter::Continuation(); 
    }

    interpreter::PredicateReference visitAsUserPredicate(
        const PredicateRef &pr
    ) {
        size_t predicateIndex = getPredicateIndex(pr.name);

        std::vector<interpreter::MatcherValue> arguments;
        const PredicateDecl &pDecl = ast.resolvePredicateRef(pr).getDeclaration();
        for(int i=0; i<pDecl.parameters.size(); ++i) {
            auto loweredCtorRef = visit(pr.arguments[i], pDecl.parameters[i].type);
            arguments.push_back(loweredCtorRef);
        }
        
        return interpreter::PredicateReference(predicateIndex, arguments);
    }

    interpreter::Expression visit(const PredicateRef &pr) {
        std::vector<interpreter::MatcherValue> arguments;
        const PredicateDecl &pDecl = ast.resolvePredicateRef(pr).getDeclaration();
        for(int i=0; i<pDecl.parameters.size(); ++i) {
            auto loweredCtorRef = visit(pr.arguments[i], pDecl.parameters[i].type);
            arguments.push_back(loweredCtorRef);
        }
    
        return ast.resolvePredicateRef(pr).match<interpreter::Expression>(
        [&](const UserPredicate *) {
            size_t predicateIndex = getPredicateIndex(pr.name);
            return interpreter::Expression(
                interpreter::PredicateReference(predicateIndex, arguments));
        },
        [&](const BuiltinPredicate *bp) {
            auto resolved = interpreter::getBuiltinPredicateByName(bp->declaration.name.string());
            return interpreter::Expression(
                interpreter::BuiltinPredicateReference(resolved, arguments));
        });
    }

    interpreter::HandlerExpression visitAsHandlerExpr(const PredicateRef &pr) {
        std::vector<interpreter::MatcherValue> arguments;
        const PredicateDecl &pDecl = ast.resolvePredicateRef(pr).getDeclaration();
        for(int i=0; i<pDecl.parameters.size(); ++i) {
            auto loweredCtorRef = visit(pr.arguments[i], pDecl.parameters[i].type);
            arguments.push_back(loweredCtorRef);
        }
    
        return ast.resolvePredicateRef(pr).match<interpreter::HandlerExpression>(
        [&](const UserPredicate *) {
            size_t predicateIndex = getPredicateIndex(pr.name);
            return interpreter::HandlerExpression(
                interpreter::PredicateReference(predicateIndex, arguments));
        },
        [&](const BuiltinPredicate *bp) {
            auto resolved = interpreter::getBuiltinPredicateByName(bp->declaration.name.string());
            return interpreter::HandlerExpression(
                interpreter::BuiltinPredicateReference(resolved, arguments));
        });
    }

    interpreter::EffectImplHead visit(const EffectImplHead &eih) {
        const EffectCtor &eCtor = ast.resolveEffectCtorRef(
            eih.effectName,
            eih.ctorName);
        auto [effectIndex, eCtorIndex] = getEffectIndices(
            eih.effectName,
            eih.ctorName);
        
        std::vector<interpreter::MatcherValue> arguments;
        arguments.reserve(eCtor.parameters.size());
        for(int i=0; i<eCtor.parameters.size(); ++i) {
            auto loweredCtorRef = visit(eih.arguments[i], eCtor.parameters[i].type);
            arguments.push_back(loweredCtorRef);
        }

        return interpreter::EffectImplHead(effectIndex, eCtorIndex, arguments);
    }

    interpreter::EffectCtorRef visit(const EffectCtorRef &ecr) {
        auto eih = visit(EffectImplHead(ecr));
        auto continuation = visit(ecr.getContinuation());
        return interpreter::EffectCtorRef(
            eih.effectIndex,
            eih.effectCtorIndex,
            eih.arguments,
            continuation);
    }

    interpreter::Conjunction visit(const Conjunction &conj) {
        return interpreter::Conjunction(
            visit(conj.getLeft()),
            visit(conj.getRight())
        );
    }

    interpreter::HandlerConjunction visit(const HandlerConjunction &hConj) {
        return interpreter::HandlerConjunction(
            visit(hConj.getLeft()),
            visit(hConj.getRight())
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

    interpreter::HandlerExpression visit(const HandlerExpression &hExpr) {
        return hExpr.match<interpreter::HandlerExpression>(
        [&](TruthLiteral tl) { return interpreter::HandlerExpression(visit(tl)); },
        [&](Continuation k) {return interpreter::HandlerExpression(visit(k)); },
        [&](PredicateRef pr) { return interpreter::HandlerExpression(visitAsHandlerExpr(pr)); },
        [&](EffectCtorRef ecr) { return interpreter::HandlerExpression(visit(ecr)); },
        [&](HandlerConjunction hConj) { return interpreter::HandlerExpression(visit(hConj)); }
        );
    }

    interpreter::Implication visit(const Implication &impl) {
        enclosingImplication = impl;
        auto head = visitAsUserPredicate(impl.head);
        auto body = visit(impl.body);
        enclosingImplication = Optional<Implication>();

        size_t variableCount = getVariables(ast, impl).size();
        return interpreter::Implication(head, body, variableCount);
    }

    interpreter::EffectImplication visit(const EffectImplication &eImpl) {
        auto head = visit(eImpl.head);
        auto body = visit(eImpl.body);
        // TODO: variables in handlers?
        return interpreter::EffectImplication(head, body, 0);
    }

    interpreter::UserHandler visit(const Handler &h) {
        size_t effectIndex = getEffectIndex(h.effect);

        std::vector<interpreter::EffectImplication> implications;
        implications.reserve(h.implications.size());
        for(const EffectImplication &hImpl : h.implications) {
            implications.push_back(visit(hImpl));
        }

        return interpreter::UserHandler(effectIndex, implications);
    }

    interpreter::Predicate visit(const UserPredicate &up) {
        std::vector<interpreter::Implication> implications;
        implications.reserve(up.implications.size());
        for(const Implication &impl : up.implications) {
            implications.push_back(visit(impl));
        }

        std::vector<interpreter::UserHandler> handlers;
        handlers.reserve(up.handlers.size());
        for(const Handler &h : up.handlers) {
            handlers.push_back(visit(h));
        }

        return interpreter::Predicate(implications, handlers);
    }

    void visit(const PredicateDecl &pd) { assert(false && "not implemented"); };

    void visit(const TypeDecl &td) { assert(false && "not implemented"); };

    void visit(const CtorParameter &cp) { assert(false && "not implemented"); };

    void visit(const Constructor &ctor) { assert(false && "not implemented"); };

    void visit(const ConstructorRef &cr) { assert(false && "not implemented"); };

    void visit(const Value &val) { assert(false && "not implemented"); };

    void visit(const Type &t) { assert(false && "not implemented"); };

private:
    // This should only be called with the name of user-defined predicates.
    size_t getPredicateIndex(const Name<Predicate> &pn) {
        // TODO: it should be possible to do this in logarithmic time,
        // but the current implementation is linear.
        auto x = std::find_if(
            ast.predicates.begin(),
            ast.predicates.end(),
            [&](const UserPredicate &up) { return up.declaration.name == pn; });

        assert(x != ast.predicates.end());
        return x - ast.predicates.begin();
    }

    size_t getTypeConstructorIndex(const Name<Type> &tr, const ConstructorRef &cr) {
        auto type = std::find_if(
            ast.types.begin(),
            ast.types.end(),
            [&](const Type &type) { return type.declaration.name == tr; });
        
        assert(type != ast.types.end());
        return getConstructorIndex(*type, cr);
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

    size_t getEffectIndex(const EffectRef &er) {
        // search for a user-defined effect type with the given name.
        auto effect = std::find_if(
            ast.effects.begin(),
            ast.effects.end(),
            [&](const Effect &e) { return er == e.declaration.name; });
        
        if(effect != ast.effects.end()) {
            return effect - ast.effects.begin() + TypedAST::builtinEffects.size();
        }

        // If the effect type is not user-defined, it must be builtin.
        effect = std::find_if(
            builtinEffects.begin(),
            builtinEffects.end(),
            [&](const Effect &e) { return er == e.declaration.name; });
        assert(effect != builtinEffects.end());
        return effect - builtinEffects.begin();
    }

    std::pair<size_t, size_t> getEffectIndices(
        const EffectRef &effectName,
        const Name<EffectCtor> ctorName
    ) {
        auto effect = std::find_if(
            ast.effects.begin(),
            ast.effects.end(),
            [&](const Effect &e) { return effectName == e.declaration.name; });

        if(effect != ast.effects.end()) {
            auto eCtor = std::find_if(
                effect->constructors.begin(),
                effect->constructors.end(),
                [&](const EffectCtor &eCtor) { return ctorName == eCtor.name; });
            
            assert(eCtor != effect->constructors.end());

            return {
                effect - ast.effects.begin() + TypedAST::builtinEffects.size(),
                eCtor - effect->constructors.begin()
            };
        }

        effect = std::find_if(
            builtinEffects.begin(),
            builtinEffects.end(),
            [&](const Effect &e) { return e.declaration.name == effectName; });
        
        // The effect must either be in the AST or a builtin effect.
        assert(effect != builtinEffects.end());
        
        auto eCtor = std::find_if(
            effect->constructors.begin(),
            effect->constructors.end(),
            [&](const EffectCtor &eCtor) { return ctorName == eCtor.name; });
        
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

    for(const auto &p : ast.predicates) {
        interpreter::Predicate lowered = lowerer.visit(p);
        loweredPredicates.push_back(lowered);

        predicateNameTable.push_back(p.declaration.name.string());

        if(p.declaration.name == Name<Predicate>("main")) {
            // If main did take arguments, this would be the place to
            // pass them to the program!
            PredicateRef mainRef = PredicateRef(p.declaration.name.string(), {});
            main = lowerer.visitAsUserPredicate(mainRef);
        }
    }

    return interpreter::Program(
        loweredPredicates,
        main,
        predicateNameTable,
        config);
}
