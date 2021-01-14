#include <algorithm>
#include "SemAna/Predicates.h"

static void checkPredicateArgumentTypes(
    const Program &program,
    const ErrorEmitter &error,
    const Predicate &predicate,
    const PredicateRef &pRef
) {
    // TODO: this can probably be collapsed into Sema::visit(ConstructorRef)
    assert(predicate.name.parameters.size() == pRef.arguments.size());
    auto parameter = predicate.name.parameters.begin();
    for(const auto &argument : pRef.arguments) {
        argument.switchOver(
        [](AnonymousVariable) {},
        [&](ConstructorRef cr) {
            const auto typeCandidates = program.types.find(parameter->name);
            if(typeCandidates == program.types.end()) {
                // TODO: emit error for non-existent parameter type.
                // Once SemAna migrates to visitor pattern, this check should be
                // applied to the PredicateDecl.
                return;
            }
            const Type &type = typeCandidates->second;
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
            }
        });
        ++parameter;
    }
}

class Sema: ASTVisitor<void> {
    const Program &program;
    const ErrorEmitter &error;

public:
    Sema(const Program & program, const ErrorEmitter &error):
        program(program), error(error) {}
    
    void visit(const TruthLiteral &tl) override {}

    void visit(const PredicateDecl &pd) override {
        for(const TypeRef &typeRef : pd.parameters) {
            visit(typeRef);
        }
    }

    void visit(const PredicateRef &pn) override {
        auto p = program.predicates.find(pn.name);
        if(p == program.predicates.end()) {
            error.emit(
                pn.location,
                ErrorMessage::undefined_predicate,
                pn.name.string());
        } else {
            checkPredicateArgumentTypes(program, error, p->second, pn);
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
        visit(impl.lhs);
        visit(impl.rhs);
    }

    void visit(const Predicate &p) override {
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

    void visit(const ConstructorRef &ctor) override {}

    void visit(const Value &val) override {
        val.switchOver(
        [&](AnonymousVariable av) { visit(av); },
        [&](ConstructorRef cr) { visit(cr); }
        );
    }

    void visit(const Type &type) override {
        visit(type.declaration);
        for(const Constructor &ctor : type.constructors) {
            visit(ctor);
        }
    }
};

void Program::checkAll() {
    Sema sema(*this, error);

    for(const auto &pair : types) {
        sema.visit(pair.second);
    }

    for(const auto &pair : predicates) {
        sema.visit(pair.second);
    }
}
