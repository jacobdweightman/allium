#include <algorithm>
#include <string>

#include "Parser/AST.h"
#include "Parser/Builtins.h"
#include "SemAna/Builtins.h"
#include "SemAna/Predicates.h"
#include "SemAna/TypedAST.h"
#include "Utils/VectorUtils.h"

using namespace parser;

class SemAna {
    const AST &ast;
    const ErrorEmitter &error;

    // The type-checked and raised type definitions.
    mutable std::vector<TypedAST::Type> raisedTypes;

    // The type-checked and raised effect definitions.
    mutable std::vector<TypedAST::Effect> raisedEffects;

    /// The predicate definition enclosing the current AST node being analyzed, if there is one.
    mutable Optional<Predicate> enclosingPredicate;

    /// The lexical scope enclosing the current AST node being analyzed.
    mutable Optional<TypedAST::Scope> enclosingScope;

    /// True if the current AST node must not contain variable definitions. This
    /// should be true inside an argument passed to an `in` parameter outside of
    /// the predicate definition/effect handler, and false otherwise.
    mutable bool isInputOnly = false;

    /// The type definition enclosing the current AST node being analyzed, if there is one.
    mutable Optional<Type> enclosingType;

    /// The inferred type of a value being analyzed.
    mutable Optional<Type> inferredType;

public:
    SemAna(const AST &ast, const ErrorEmitter &error):
        ast(ast), error(error) {}

    TypedAST::TruthLiteral visit(const TruthLiteral &tl) {
        return TypedAST::TruthLiteral(tl.value);
    }

    Optional<TypedAST::PredicateDecl> visit(const PredicateDecl &pd) {
        const auto originalDeclaration = std::find_if(
            ast.predicates.begin(),
            ast.predicates.end(),
            [&](Predicate p) {
                return p.name.name == pd.name;
            });

        if(originalDeclaration->name.location != pd.location) {
            error.emit(
                pd.location,
                ErrorMessage::predicate_redefined,
                pd.name.string(),
                originalDeclaration->name.location.toString());
            return Optional<TypedAST::PredicateDecl>();
        }

        return both(
            fullMap<Parameter, TypedAST::Parameter>(
                pd.parameters,
                [&](Parameter tr) -> Optional<TypedAST::Parameter> { return visit(tr); }
            ),
            fullMap<EffectRef, TypedAST::EffectRef>(
                pd.effects,
                [&](EffectRef er) { return visit(er); }
            )
        ).map<TypedAST::PredicateDecl>(
            [&](auto pair) {
                return TypedAST::PredicateDecl(pd.name.string(), pair.first, pair.second);
            }
        );
    }

    Optional<TypedAST::PredicateRef> visit(const PredicateRef &pr) {
        Predicate enclosingPred;
        if(enclosingPredicate.unwrapGuard(enclosingPred)) {
            assert(enclosingPredicate && "enclosingPredicate not set!");
            return Optional<TypedAST::PredicateRef>();
        }

        PredicateDecl pDecl;
        if(ast.resolvePredicateRef(pr).unwrapGuard(pDecl)) {
            error.emit(
                pr.location,
                ErrorMessage::undefined_predicate,
                pr.name.string());
            return Optional<TypedAST::PredicateRef>();
        }

        for(const auto &unhandledEffect : pDecl.effects) {
            bool handledInEnclosing = std::find_if(
                enclosingPred.handlers.begin(),
                enclosingPred.handlers.end(),
                [&](const Handler &h) { return h.effect.name == unhandledEffect.name; }
            ) != enclosingPred.handlers.end();

            bool handledAboveEnclosing = std::find_if(
                enclosingPred.name.effects.begin(),
                enclosingPred.name.effects.end(),
                [&](const EffectRef &er) { return er.name == unhandledEffect.name; }
            ) != enclosingPred.name.effects.end();

            if(!handledInEnclosing && !handledAboveEnclosing) {
                error.emit(
                    pr.location,
                    ErrorMessage::effect_from_predicate_unhandled,
                    enclosingPred.name.name.string(),
                    unhandledEffect.name.string(),
                    pr.name.string());
                return Optional<TypedAST::PredicateRef>();
            }
        }

        if(pDecl.parameters.size() != pr.arguments.size()) {
            error.emit(
                pr.location,
                ErrorMessage::predicate_argument_count,
                pr.name.string(),
                std::to_string(pDecl.parameters.size()));
            return Optional<TypedAST::PredicateRef>();
        }

        auto parameter = pDecl.parameters.begin();
        auto arguments = compactMap<Value, TypedAST::Value>(
            pr.arguments,
            [&](Value argument) {
                inferredType = ast.resolveTypeRef(parameter->name);
                if(!inferredType) {
                    ++parameter;
                    return Optional<TypedAST::Value>();
                }
                isInputOnly = parameter->isInputOnly && enclosingPred.name.name != pr.name;
                auto raisedArg = visit(argument);
                isInputOnly = false;
                inferredType = Optional<Type>();
                ++parameter;
                return raisedArg;
            }
        );

        return TypedAST::PredicateRef(pr.name.string(), arguments);
    }

    Optional<TypedAST::EffectCtorRef> visit(const EffectCtorRef &ecr) {
        Predicate p;
        if(enclosingPredicate.unwrapGuard(p)) {
            assert(enclosingPredicate && "enclosingPredicate not set!");
            return Optional<TypedAST::EffectCtorRef>();
        }

        std::vector<EffectConstructor>::const_iterator eCtor;
        auto effect = std::find_if(
            p.name.effects.begin(),
            p.name.effects.end(),
            [&](const EffectRef &er) {
                const Effect *e;
                if(ast.resolveEffectRef(er).unwrapGuard(e)) {
                    // The effect is undefined.
                    return false;
                }
                eCtor = std::find_if(
                    e->constructors.begin(),
                    e->constructors.end(),
                    [&](const EffectConstructor &eCtor) { return eCtor.name == ecr.name; }
                );
                return eCtor != e->constructors.end();
            });

        if(effect == p.name.effects.end()) {
            error.emit(
                ecr.location,
                ErrorMessage::effect_unknown,
                ecr.name.string(),
                p.name.name.string());
            return Optional<TypedAST::EffectCtorRef>();
        }

        if(ecr.arguments.size() != eCtor->parameters.size()) {
            error.emit(
                ecr.location,
                ErrorMessage::effect_argument_count,
                eCtor->name.string(),
                effect->name.string(),
                std::to_string(eCtor->parameters.size()));
            return Optional<TypedAST::EffectCtorRef>();
        }

        auto parameter = eCtor->parameters.begin();
        auto raisedArguments = compactMap<Value, TypedAST::Value>(
            ecr.arguments,
            [&](Value argument) -> Optional<TypedAST::Value> {
                inferredType = ast.resolveTypeRef(parameter->name);
                if(!inferredType) {
                    ++parameter;
                    return Optional<TypedAST::Value>();
                }
                isInputOnly = parameter->isInputOnly; // TODO: must be false if inside handler
                Optional<TypedAST::Value> raisedArg = visit(argument);
                isInputOnly = false;
                inferredType = Optional<Type>();
                ++parameter;
                return raisedArg;
            }
        );

        return TypedAST::EffectCtorRef(
            effect->name.string(),
            eCtor->name.string(),
            raisedArguments,
            ecr.location);
    }

    Optional<TypedAST::Conjunction> visit(const Conjunction &conj) {
        std::unique_ptr<TypedAST::Expression> left, right;
        if( visit(conj.getLeft()).unwrapGuard(left) ||
            visit(conj.getRight()).unwrapGuard(right)) {
                return Optional<TypedAST::Conjunction>();
        }
        return TypedAST::Conjunction(*left, *right);
    }

    Optional<TypedAST::Expression> visit(const Expression &expr) {
        return expr.match<Optional<TypedAST::Expression> >(
        [&](TruthLiteral tl) { return TypedAST::Expression(visit(tl)); },
        [&](PredicateRef pr) {
            return visit(pr).map<TypedAST::Expression>(
                [](TypedAST::PredicateRef tpr) { return TypedAST::Expression(tpr); }
            );
        },
        [&](EffectCtorRef ecr) {
            return visit(ecr).map<TypedAST::Expression>(
                [](TypedAST::EffectCtorRef ecr) { return TypedAST::Expression(ecr); }
            );
        },
        [&](Conjunction conj) {
            return visit(conj).map<TypedAST::Expression>(
                [](TypedAST::Conjunction tconj) { return TypedAST::Expression(tconj); }
            );
        });
    }

    Optional<TypedAST::Implication> visit(const Implication &impl) {
        enclosingScope = TypedAST::Scope();
        auto head = visit(impl.lhs);
        auto body = visit(impl.rhs);
        enclosingScope = Optional<TypedAST::Scope>();

        return head.flatMap<TypedAST::Implication>([&](TypedAST::PredicateRef h) {
            return body.map<TypedAST::Implication>([&](TypedAST::Expression b) {
                return TypedAST::Implication(h, b);
            });
        });
    }

    Optional<TypedAST::Predicate> visit(const Predicate &p) {
        enclosingPredicate = p;

        std::unique_ptr<TypedAST::PredicateDecl> declaration;
        if(visit(p.name).unwrapGuard(declaration)) {
            enclosingPredicate = Optional<Predicate>();
            return Optional<TypedAST::Predicate>();
        }

        std::vector<TypedAST::Implication> raisedImplications;
        for(const auto &impl : p.implications) {
            if(impl.lhs.name != p.name.name) {
                error.emit(
                    impl.lhs.location,
                    ErrorMessage::impl_head_mismatches_predicate,
                    p.name.name.string());
            }
            visit(impl).map([&](TypedAST::Implication impl) {
                raisedImplications.push_back(impl);
            });
        }

        std::vector<TypedAST::Handler> raisedHandlers;
        for(const auto &h : p.handlers) {
            visit(h).map([&](TypedAST::Handler h) {
                raisedHandlers.push_back(h);
            });
        }

        enclosingPredicate = Optional<Predicate>();
        return TypedAST::Predicate(
            *declaration,
            raisedImplications,
            raisedHandlers);
    }

    Optional<TypedAST::TypeDecl> visit(const TypeDecl &td) {
        if(nameIsBuiltinType(td.name)) {
            error.emit(
                td.location,
                ErrorMessage::builtin_redefined,
                td.name.string());
            return Optional<TypedAST::TypeDecl>();
        }

        const auto originalDeclaration = std::find_if(
            ast.types.begin(),
            ast.types.end(),
            [&](Type type) {
                return type.declaration.name == td.name;
            })->declaration;

        if(originalDeclaration != td) {
            error.emit(
                td.location,
                ErrorMessage::type_redefined,
                td.name.string(),
                originalDeclaration.location.toString());
            return Optional<TypedAST::TypeDecl>();
        }

        return TypedAST::TypeDecl(td.name.string());
    }

    Optional<TypedAST::Parameter> visit(const Parameter &cp) {
        if(nameIsBuiltinType(cp.name)) {
            return TypedAST::Parameter(cp.name.string(), cp.isInputOnly);
        }

        if(!ast.resolveTypeRef(cp.name)) {
            error.emit(
                cp.location,
                ErrorMessage::undefined_type,
                cp.name.string());
            return Optional<TypedAST::Parameter>();
        }
        return TypedAST::Parameter(cp.name.string(), cp.isInputOnly);
    }

    Optional<TypedAST::CtorParameter> visit(const CtorParameter &cp) {
        if(nameIsBuiltinType(cp.name)) {
            return TypedAST::CtorParameter(cp.name.string());
        }

        if(!ast.resolveTypeRef(cp.name)) {
            error.emit(
                cp.location,
                ErrorMessage::undefined_type,
                cp.name.string());
            return Optional<TypedAST::CtorParameter>();
        }
        return TypedAST::CtorParameter(cp.name.string());
    }

    Optional<TypedAST::Constructor> visit(const Constructor &ctor) {
        return fullMap<CtorParameter, TypedAST::CtorParameter>(
            ctor.parameters,
            [&](CtorParameter param) { return visit(param); }
        ).map<TypedAST::Constructor>(
            [&](std::vector<TypedAST::CtorParameter> raisedParameters) {
                return TypedAST::Constructor(ctor.name.string(), raisedParameters);
            }
        );
    }

    Optional<TypedAST::Variable> visitNamedValueAsVariable(const NamedValue &v) {
        Type type;
        if(inferredType.unwrapGuard(type)) {
            assert(inferredType && "inferred type not set!");
            return Optional<TypedAST::Variable>();
        }

        auto raisedType = std::find_if(
            raisedTypes.begin(),
            raisedTypes.end(),
            [&](const TypedAST::Type &rt) { return rt.declaration.name.string() == type.declaration.name.string(); });

        if(raisedType == raisedTypes.end()) {
            raisedType = std::find_if(
                TypedAST::builtinTypes.begin(),
                TypedAST::builtinTypes.end(),
                [&](const TypedAST::Type &rt) { return rt.declaration.name.string() == type.declaration.name.string(); });
            assert(raisedType != TypedAST::builtinTypes.end());
        }

        TypedAST::Scope *scope;
        if(enclosingScope.unwrapGuard(scope)) {
            assert(enclosingScope && "Scope not initialized!");
            return Optional<TypedAST::Variable>();
        }

        if(isInputOnly && v.isDefinition) {
            error.emit(v.location, ErrorMessage::input_only_argument_contains_variable_definition, v.name.string());
            return Optional<TypedAST::Variable>();
        }

        if(v.isDefinition) {
            auto name = Name<TypedAST::Variable>(v.name.string());
            if(scope->find(name) == scope->end()) {
                scope->insert({ name, *raisedType });
            } else {
                error.emit(v.location, ErrorMessage::variable_redefined, v.name.string());
                return Optional<TypedAST::Variable>();
            }
        } else {
            auto name = Name<TypedAST::Variable>(v.name.string());
            auto iterVariableInfo = scope->find(name);
            if(iterVariableInfo == scope->end()) {
                error.emit(
                    v.location,
                    ErrorMessage::unknown_constructor_or_variable,
                    v.name.string(),
                    type.declaration.name.string());
                return Optional<TypedAST::Variable>();
            }
            if(*raisedType != iterVariableInfo->second) {
                error.emit(
                    v.location,
                    ErrorMessage::variable_type_mismatch,
                    v.name.string(),
                    iterVariableInfo->second.declaration.name.string(),
                    type.declaration.name.string());
                return Optional<TypedAST::Variable>();
            }
        }

        return TypedAST::Variable(
            v.name.string(),
            raisedType->declaration.name,
            v.isDefinition);
    }

    Optional<TypedAST::ConstructorRef> visitNamedValueAsConstructorRef(const Type &t, const Constructor &ctor, const NamedValue &cr) {
        if(ctor.parameters.size() != cr.arguments.size()) {
            error.emit(
                cr.location,
                ErrorMessage::constructor_argument_count,
                cr.name.string(),
                t.declaration.name.string(),
                std::to_string(ctor.parameters.size()));
            return Optional<TypedAST::ConstructorRef>();
        }

        auto parameter = ctor.parameters.begin();
        auto raisedArguments = compactMap<Value, TypedAST::Value>(
            cr.arguments,
            [&](Value argument) -> Optional<TypedAST::Value> {
                inferredType = ast.resolveTypeRef(parameter->name);
                if(!inferredType) {
                    ++parameter;
                    return Optional<TypedAST::Value>();
                }
                Optional<TypedAST::Value> raisedArg = visit(argument);
                inferredType = Optional<Type>();
                ++parameter;
                return raisedArg;
            }
        );

        return TypedAST::ConstructorRef(cr.name.string(), raisedArguments);
    }

    Optional<TypedAST::Value> visit(const NamedValue &val) {
        Type type;
        if(inferredType.unwrapGuard(type)) {
            assert(false && "type inference failed.");
            return Optional<TypedAST::Value>();
        }

        if(val.name.string() == "_") {
            Name<TypedAST::Type> tr(type.declaration.name.string());
            return TypedAST::Value(TypedAST::AnonymousVariable(tr));
        }

        auto ctor = std::find_if(
            type.constructors.begin(),
            type.constructors.end(),
            [&](const Constructor &ctor) { return ctor.name.string() == val.name.string(); });

        if(ctor == type.constructors.end()) {
            if(val.arguments.size() > 0) {
                error.emit(
                    val.location,
                    ErrorMessage::unknown_constructor,
                    val.name.string(),
                    type.declaration.name.string());
                return Optional<TypedAST::Value>();
            } else {
                return visitNamedValueAsVariable(val).map<TypedAST::Value>([](TypedAST::Variable var) {
                    return TypedAST::Value(var);
                });
            }
        } else {
            return visitNamedValueAsConstructorRef(type, *ctor, val).map<TypedAST::Value>([](TypedAST::ConstructorRef cr) {
                return TypedAST::Value(cr);
            });
        }
    }

    Optional<TypedAST::Value> visit(const StringLiteral &str) {
        Type type;
        if(inferredType.unwrapGuard(type)) {
            assert(false && "type inference failed.");
            return Optional<TypedAST::Value>();
        }

        // TODO: allow user-defined types to be representable as string literals
        if(type.declaration.name != "String") {
            error.emit(
                str.location,
                ErrorMessage::string_literal_not_convertible,
                type.declaration.name.string());
            return Optional<TypedAST::Value>();
        }

        return TypedAST::Value(TypedAST::StringLiteral(str.text));
    }

    Optional<TypedAST::Value> visit(const IntegerLiteral &i) {
        Type type;
        if(inferredType.unwrapGuard(type)) {
            assert(false && "type inference failed.");
            return Optional<TypedAST::Value>();
        }

        // TODO: allow user-defined types to be representable as int literals
        if(type.declaration.name != "Int") {
            error.emit(
                i.location,
                ErrorMessage::int_literal_not_convertible,
                type.declaration.name.string());
            return Optional<TypedAST::Value>();
        }

        return TypedAST::Value(TypedAST::IntegerLiteral(i.value));
    }

    Optional<TypedAST::Value> visit(const Value &val) {
        return val.match<Optional<TypedAST::Value>>(
        [&](NamedValue nv) { return visit(nv); },
        [&](StringLiteral str) { return visit(str); },
        [&](IntegerLiteral i) { return visit(i); }
        );
    }

    Optional<TypedAST::Type> visit(const Type &type) {
        enclosingType = type;

        TypedAST::TypeDecl raisedDeclaration;
        if(visit(type.declaration).unwrapGuard(raisedDeclaration)) {
            return Optional<TypedAST::Type>();
        }

        std::vector<TypedAST::Constructor> raisedCtors;
        for(const Constructor &ctor : type.constructors) {
            visit(ctor).map([&](TypedAST::Constructor raisedCtor) {
                raisedCtors.push_back(raisedCtor);
            });
        }
        enclosingType = Optional<Type>();

        return TypedAST::Type(raisedDeclaration, raisedCtors);
    }

    Optional<TypedAST::EffectRef> visit(const EffectRef &er) {
        // Short-circuit for builtin types
        if(er.name == "IO") {
            return TypedAST::EffectRef("IO");
        }

        if(!ast.resolveEffectRef(er)) {
            error.emit(
                er.location,
                ErrorMessage::effect_type_undefined,
                er.name.string());
            return Optional<TypedAST::EffectRef>();
        }
        return TypedAST::EffectRef(er.name.string());
    }

    Optional<TypedAST::EffectDecl> visit(const EffectDecl &decl) {
        // TODO: builtin effects?

        const auto originalDeclaration = std::find_if(
            ast.effects.begin(),
            ast.effects.end(),
            [&](Effect e) {
                return e.declaration.name == decl.name;
            })->declaration;

        if(originalDeclaration != decl) {
            error.emit(
                decl.location,
                ErrorMessage::effect_redefined,
                decl.name.string(),
                originalDeclaration.location.toString());
            return Optional<TypedAST::EffectDecl>();
        }

        return TypedAST::EffectDecl(decl.name.string());
    }

    Optional<TypedAST::EffectCtor> visit(const EffectConstructor &eCtor) {
        return fullMap<Parameter, TypedAST::Parameter>(
            eCtor.parameters,
            [&](Parameter param) { return visit(param); }
        ).map<TypedAST::EffectCtor>(
            [&](std::vector<TypedAST::Parameter > raisedParameters) {
                return TypedAST::EffectCtor(eCtor.name.string(), raisedParameters);
            }
        );
    }

    Optional<TypedAST::Effect> visit(const Effect &effect) {
        TypedAST::EffectDecl raisedDeclaration;
        if(visit(effect.declaration).unwrapGuard(raisedDeclaration)) {
            return Optional<TypedAST::Effect>();
        }

        std::vector<TypedAST::EffectCtor> raisedCtors;
        for(const EffectConstructor &eCtor : effect.constructors) {
            visit(eCtor).map([&](TypedAST::EffectCtor raisedCtor) {
                raisedCtors.push_back(raisedCtor);
            });
        }

        return TypedAST::Effect(raisedDeclaration, raisedCtors);
    }

    Optional<TypedAST::Handler> visit(const Handler &h) {
        assert(false && "Handlers aren't implemented yet!");
        return TypedAST::Handler();
    }

    TypedAST::AST visit(const AST &ast) {
        raisedTypes = compactMap<Type, TypedAST::Type>(
            ast.types,
            [&](Type type) { return visit(type); }
        );

        raisedEffects = compactMap<Effect, TypedAST::Effect>(
            ast.effects,
            [&](Effect effect) { return visit(effect); }
        );

        auto raisedPredicates = compactMap<Predicate, TypedAST::Predicate>(
            ast.predicates,
            [&](Predicate predicate) { return visit(predicate); }
        );

        return TypedAST::AST(raisedTypes, raisedEffects, raisedPredicates);
    }
};

static_assert(has_all_visitors<SemAna>(), "SemAna missing visitor(s).");

TypedAST::AST checkAll(const AST &ast, ErrorEmitter &error) {
    return SemAna(ast, error).visit(ast);
}
