#include <algorithm>
#include <string>
#include "SemAna/Predicates.h"

class SemAna: ASTVisitor<void> {
    const Program &program;
    const ErrorEmitter &error;

    /// The predicate definition enclosing the current AST node being analyzed, if there is one.
    mutable Optional<Predicate> enclosingPredicate;

    /// The implication enclosing the current AST node being analyzed, if there is one.
    mutable Optional<Implication> enclosingImplication;

    /// Whether or not the current AST node being analyzed occurs in the body of an implication.
    /// TODO: remove this when variables can be defined in implication bodies.
    mutable bool inBody = false;

    /// The type definition enclosing the current AST node being analyzed, if there is one.
    mutable Optional<Type> enclosingType;

    /// The inferred type of a value being analyzed.
    mutable Optional<Type> inferredType;

public:
    SemAna(const Program & program, const ErrorEmitter &error):
        program(program), error(error) {}
    
    void visit(const TruthLiteral &tl) override {}

    void visit(const PredicateDecl &pd) override {
        for(const TypeRef &typeRef : pd.parameters) {
            visit(typeRef);
        }
    }

    void visit(const PredicateRef &pr) override {
        auto p = program.predicates.find(pr.name);
        if(p == program.predicates.end()) {
            error.emit(
                pr.location,
                ErrorMessage::undefined_predicate,
                pr.name.string());
            return;
        }

        const Predicate &predicate = p->second;

        if(predicate.name.parameters.size() != pr.arguments.size()) {
            error.emit(
                pr.location,
                ErrorMessage::predicate_argument_count,
                pr.name.string(),
                std::to_string(predicate.name.parameters.size()));
            return;
        }

        auto parameter = predicate.name.parameters.begin();
        for(const auto &argument : pr.arguments) {
            inferredType = program.resolveTypeRef(*parameter);
            visit(argument);
            inferredType = Optional<Type>();
            ++parameter;
        }
    }

    void visit(const Conjunction &conj) override {
        visit(conj.getLeft());
        visit(conj.getRight());
    }

    void visit(const Expression &expr) override {
        expr.switchOver(
        [&](TruthLiteral tl) { visit(tl); },
        [&](PredicateRef pr) { visit(pr); },
        [&](Conjunction conj) { visit(conj); }
        );
    }

    void visit(const Implication &impl) override {
        program.scopes.insert({ impl, Program::Scope() });
        enclosingImplication = impl;
        visit(impl.lhs);
        inBody = true;
        visit(impl.rhs);
        inBody = false;
        enclosingImplication = Optional<Implication>();
    }

    void visit(const Predicate &p) override {
        enclosingPredicate = p;
        visit(p.name);
        for(const auto &impl : p.implications) {
            if(impl.lhs.name != p.name.name) {
                error.emit(
                    impl.lhs.location,
                    ErrorMessage::impl_head_mismatches_predicate,
                    p.name.name.string());
            }
            visit(impl);
        }
        enclosingPredicate = Optional<Predicate>();
    }

    void visit(const TypeDecl &td) override {}

    void visit(const TypeRef &typeRef) override {
        const auto typeCandidates = program.types.find(typeRef.name);
        if(typeCandidates == program.types.end()) {
            error.emit(
                typeRef.location,
                ErrorMessage::undefined_type,
                typeRef.name.string());
        }
    }

    void visit(const Constructor &ctor) override {
        for(const auto &param : ctor.parameters) {
            visit(param);
        }
    }

    void visit(const AnonymousVariable &av) override {}

    void visit(const Variable &v) override {
        Implication impl;
        if(enclosingImplication.unwrapGuard(impl)) {
            assert(false && "A variable must not be defined outside an implication.");
            return;
        }
        auto iter = program.scopes.find(impl);
        assert(iter != program.scopes.end());
        Program::Scope &scope = iter->second;

        Type type;
        if(inferredType.unwrapGuard(type)) {
            assert(inferredType && "inferred type not set!");
            return;
        }

        if(v.isDefinition) {
            if(inBody) {
                error.emit(v.location, ErrorMessage::variable_defined_in_body, v.name.string());
            }

            if(scope.find(v.name) == scope.end()) {
                scope.insert({ v.name, type });
            } else {
                error.emit(v.location, ErrorMessage::variable_redefined, v.name.string());
            }
        } else {
            auto iterType = scope.find(v.name);
            if(iterType == scope.end()) {
                error.emit(
                    v.location,
                    ErrorMessage::unknown_constructor_or_variable,
                    v.name.string(),
                    type.declaration.name.string());
            } else {
                Type &variableTypeAtDefinition = iterType->second;
                if(type != variableTypeAtDefinition) {
                    error.emit(
                        v.location,
                        ErrorMessage::variable_type_mismatch,
                        v.name.string(),
                        variableTypeAtDefinition.declaration.name.string(),
                        type.declaration.name.string());
                }
            }
        }
    }

    void visit(const ConstructorRef &cr) override {
        Type type;
        if(inferredType.unwrapGuard(type)) {
            assert(false && "type inference failed.");
            return;
        }
        auto ctor = std::find_if(
            type.constructors.begin(),
            type.constructors.end(),
            [&](const Constructor &ctor) { return ctor.name == cr.name; });
        
        if(ctor == type.constructors.end()) {
            // this must be a constructor (and not a variable) if it takes parameters.
            // Otherwise, we must "fall through" and check if it's a variable since
            // we cannot distinguish variable use from constructors without parameters.
            if(cr.arguments.size() > 0) {
                error.emit(
                    cr.location,
                    ErrorMessage::unknown_constructor,
                    cr.name.string(),
                    type.declaration.name.string());
                return;
            } else {
                // There is an ambiguity in the grammar between constructors without
                // arguments and variables, so if the constructor doesn't match any
                // of the type's constructors then attempt to resolve it as a variable.
                visit(Variable(cr.name.string(), false, cr.location));
                return;
            }
        }

        if(ctor->parameters.size() != cr.arguments.size()) {
            error.emit(
                cr.location,
                ErrorMessage::constructor_argument_count,
                cr.name.string(),
                std::to_string(ctor->parameters.size()));
            return;
        }

        auto parameter = ctor->parameters.begin();
        for(const auto &argument : cr.arguments) {
            inferredType = program.resolveTypeRef(*parameter);
            visit(argument);
            inferredType = Optional<Type>();
            ++parameter;
        }
    }

    void visit(const Value &val) override {
        val.switchOver(
        [&](AnonymousVariable av) { visit(av); },
        [&](Variable v) { visit(v); },
        [&](ConstructorRef cr) { visit(cr); }
        );
    }

    void visit(const Type &type) override {
        enclosingType = type;
        visit(type.declaration);
        for(const Constructor &ctor : type.constructors) {
            visit(ctor);
        }
        enclosingType = Optional<Type>();
    }
};

void Program::checkAll() {
    SemAna semAna(*this, error);

    for(const auto &pair : types) {
        semAna.visit(pair.second);
    }

    for(const auto &pair : predicates) {
        semAna.visit(pair.second);
    }
}
