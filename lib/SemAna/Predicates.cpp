#include <algorithm>
#include <string>

#include "Parser/AST.h"
#include "SemAna/Predicates.h"
#include "SemAna/TypedAST.h"
#include "values/VectorUtils.h"

using namespace parser;

class SemAna {
    const AST &ast;
    const ErrorEmitter &error;

    // The type-checked and raised type definitions.
    mutable std::vector<TypedAST::Type> raisedTypes;

    /// The predicate definition enclosing the current AST node being analyzed, if there is one.
    mutable Optional<Predicate> enclosingPredicate;

    /// The lexical scope enclosing the current AST node being analyzed.
    mutable Optional<TypedAST::Scope> enclosingScope;

    /// Whether or not the current AST node being analyzed occurs in the body of
    /// an implication.
    mutable bool inBody = false;

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
        return fullMap<TypeRef, TypedAST::TypeRef>(
            pd.parameters,
            [&](TypeRef tr) -> Optional<TypedAST::TypeRef> { return visit(tr); }
        ).map<TypedAST::PredicateDecl>(
            [&](std::vector<TypedAST::TypeRef> parameters) {
                return TypedAST::PredicateDecl(pd.name.string(), parameters);
            }
        );
    }

    Optional<TypedAST::PredicateRef> visit(const PredicateRef &pr) {
        Predicate predicate;
        if(ast.resolvePredicateRef(pr).unwrapGuard(predicate)) {
            error.emit(
                pr.location,
                ErrorMessage::undefined_predicate,
                pr.name.string());
            return Optional<TypedAST::PredicateRef>();
        }

        if(predicate.name.parameters.size() != pr.arguments.size()) {
            error.emit(
                pr.location,
                ErrorMessage::predicate_argument_count,
                pr.name.string(),
                std::to_string(predicate.name.parameters.size()));
            return Optional<TypedAST::PredicateRef>();
        }

        auto parameter = predicate.name.parameters.begin();
        auto arguments = compactMap<Value, TypedAST::Value>(
            pr.arguments,
            [&](Value argument) {
                inferredType = ast.resolveTypeRef(*parameter);
                auto raisedArg = visit(argument);
                inferredType = Optional<Type>();
                ++parameter;
                return raisedArg;
            }
        );

        return TypedAST::PredicateRef(pr.name.string(), arguments);
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
        [&](Conjunction conj) {
            return visit(conj).map<TypedAST::Expression>(
                [](TypedAST::Conjunction tconj) { return TypedAST::Expression(tconj); }
            );
        });
    }

    Optional<TypedAST::Implication> visit(const Implication &impl) {
        enclosingScope = TypedAST::Scope();
        auto head = visit(impl.lhs);
        inBody = true;
        auto body = visit(impl.rhs);
        inBody = false;
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
        enclosingPredicate = Optional<Predicate>();
        return TypedAST::Predicate(*declaration, raisedImplications);
    }

    Optional<TypedAST::TypeDecl> visit(const TypeDecl &td) {
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

    Optional<TypedAST::TypeRef> visit(const TypeRef &typeRef) {
        if(!ast.resolveTypeRef(typeRef)) {
            error.emit(
                typeRef.location,
                ErrorMessage::undefined_type,
                typeRef.name.string());
            return Optional<TypedAST::TypeRef>();
        }
        return TypedAST::TypeRef(typeRef.name.string());
    }

    Optional<TypedAST::Constructor> visit(const Constructor &ctor) {
        return fullMap<TypeRef, TypedAST::TypeRef>(
            ctor.parameters,
            [&](TypeRef param) { return visit(param); }
        ).map<TypedAST::Constructor>(
            [&](std::vector<TypedAST::TypeRef> raisedParameters) {
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

        TypedAST::Scope *scope;
        if(enclosingScope.unwrapGuard(scope)) {
            assert(enclosingScope && "Scope not initialized!");
            return Optional<TypedAST::Variable>();
        }
 
        // A variable is existential iff it is defined in the body of an
        // implication. Record this information for each variable in the scope.
        bool isExistential;
        if(v.isDefinition) {
            auto name = Name<TypedAST::Variable>(v.name.string());
            if(scope->find(name) == scope->end()) {
                scope->insert({
                    name,
                    TypedAST::VariableInfo(*raisedType, inBody)
                });
                isExistential = inBody;
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
            isExistential = iterVariableInfo->second.isExistential;
            TypedAST::Type &variableTypeAtDefinition = iterVariableInfo->second.type;
            if(*raisedType != variableTypeAtDefinition) {
                error.emit(
                    v.location,
                    ErrorMessage::variable_type_mismatch,
                    v.name.string(),
                    variableTypeAtDefinition.declaration.name.string(),
                    type.declaration.name.string());
                return Optional<TypedAST::Variable>();
            }
        }

        return TypedAST::Variable(
            v.name.string(),
            raisedType->declaration.name,
            v.isDefinition,
            isExistential);
    }

    Optional<TypedAST::ConstructorRef> visitNamedValueAsConstructorRef(const Constructor &ctor, const NamedValue &cr) {
        if(ctor.parameters.size() != cr.arguments.size()) {
            error.emit(
                cr.location,
                ErrorMessage::constructor_argument_count,
                cr.name.string(),
                std::to_string(ctor.parameters.size()));
            return Optional<TypedAST::ConstructorRef>();
        }

        auto parameter = ctor.parameters.begin();
        auto raisedArguments = compactMap<Value, TypedAST::Value>(
            cr.arguments,
            [&](Value argument) -> Optional<TypedAST::Value> {
                inferredType = ast.resolveTypeRef(*parameter);
                Optional<TypedAST::Value> raisedArg = visit(argument);
                inferredType = Optional<Type>();
                ++parameter;
                return raisedArg;
            }
        );

        return TypedAST::ConstructorRef(cr.name.string(), raisedArguments);
    }

    Optional<TypedAST::Value> visit(const NamedValue &val) {
        if(val.name.string() == "_") {
            return TypedAST::Value(TypedAST::AnonymousVariable());
        }

        Type type;
        if(inferredType.unwrapGuard(type)) {
            assert(false && "type inference failed.");
            return Optional<TypedAST::Value>();
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
            return visitNamedValueAsConstructorRef(*ctor, val).map<TypedAST::Value>([](TypedAST::ConstructorRef cr) {
                return TypedAST::Value(cr);
            });
        }
    }

    Optional<TypedAST::Value> visit(const StringLiteral &str) {
        assert(false && "String literals are not supported yet!");
        return Optional<TypedAST::Value>();
    }

    Optional<TypedAST::Value> visit(const Value &val) {
        return val.match<Optional<TypedAST::Value>>(
        [&](NamedValue nv) { return visit(nv); },
        [&](StringLiteral str) { return visit(str); }
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

    void visit(const EffectDecl &decl) {
        assert(false && "Effects are not supported yet!");
    }

    void visit(const EffectConstructor &ctor) {
        assert(false && "Effects are not supported yet!");
    }

    void visit(const Effect &effect) {
        assert(false && "Effects are not supported yet!");
    }

    TypedAST::AST visit(const AST &ast) {
        raisedTypes = compactMap<Type, TypedAST::Type>(
            ast.types,
            [&](Type type) { return visit(type); }
        );

        for(const auto &effect : ast.effects) {
            visit(effect);
        }

        auto raisedPredicates = compactMap<Predicate, TypedAST::Predicate>(
            ast.predicates,
            [&](Predicate predicate) { return visit(predicate); }
        );

        return TypedAST::AST(raisedTypes, raisedPredicates);
    }
};

static_assert(has_all_visitors<SemAna>(), "SemAna missing visitor(s).");

TypedAST::AST checkAll(const AST &ast, ErrorEmitter &error) {
    return SemAna(ast, error).visit(ast);
}
