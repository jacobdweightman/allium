#include <algorithm>
#include "SemAna/Predicates.h"

class SemAna: ASTVisitor<void> {
    const Program &program;
    const ErrorEmitter &error;

    /// The predicate definition enclosing the current AST node being analyzed, if there is one.
    mutable Optional<Predicate> enclosingPredicate;

    /// The implication enclosing the current AST node being analyzed, if there is one.
    mutable Optional<Implication> enclosingImplication;

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
        // TODO: this assert should be a short-circuiting diagnostic
        assert(predicate.name.parameters.size() == pr.arguments.size());
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
        enclosingImplication = impl;
        visit(impl.lhs);
        visit(impl.rhs);
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
            error.emit(
                cr.location,
                ErrorMessage::unknown_constructor,
                cr.name.string(),
                type.declaration.name.string());
            return;
        }

        // TODO: this assert should be a short-circuiting diagnostic
        assert(ctor->parameters.size() == cr.arguments.size());
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
