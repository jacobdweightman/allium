#include <ostream>

#include "SemAna/ASTPrinter.h"

namespace TypedAST {

void ASTPrinter::visit(const TypeDecl &td) {
    indent();
    out << "<TypeDecl \"" << td.name << "\">\n";
}

void ASTPrinter::visit(const Constructor &ctor) {
    indent();
    out << "<Constructor \"" << ctor.name << "\">\n";
    depth++;
    for(const auto &x : ctor.parameters) visit(x);
    depth--;
}

void ASTPrinter::visit(const ConstructorRef &cr) {
    indent();
    out << "<ConstructorRef \"" << cr.name << "\">\n";
    depth++;
    for(const auto &x : cr.arguments) visit(x);
    depth--;
}

void ASTPrinter::visit(const Type &type) {
    indent();
    out << "<Type>\n";
    depth++;
    visit(type.declaration);
    for(const auto &x : type.constructors) visit(x);
    depth--;
}

void ASTPrinter::visit(const CtorParameter &cp) {
    indent();
    out << "<CtorParameter \"" << cp.type << "\">\n";
}

void ASTPrinter::visit(const EffectDecl &eDecl) {
    indent();
    out << "<EffectDecl \"" << eDecl.name << "\">\n";
}

void ASTPrinter::visit(const Parameter &p) {
    indent();
    out << "<Parameter \"" << p.type << "\"" <<
        (p.isInputOnly ? " in" : "") << ">\n";
}

void ASTPrinter::visit(const EffectCtor &eCtor) {
    indent();
    out << "<EffectCtor \"" << eCtor.name << "\">\n";
    depth++;
    for(const auto &parameter : eCtor.parameters) {
        visit(parameter);
    }
    depth--;
}

void ASTPrinter::visit(const Effect &e) {
    indent();
    out << "<Effect>\n";
    depth++;
    visit(e.declaration);
    for(const auto &ctor : e.constructors) {
        visit(ctor);
    }
    depth--;
}

void ASTPrinter::visit(const UserPredicate &up) {
    indent();
    out << "<Predicate>\n";
    depth++;
    visit(up.declaration);
    for(const auto &x : up.implications) visit(x);
    for(const auto &h : up.handlers) visit(h);
    depth--;
}

void ASTPrinter::visit(const Handler &h) {
    indent();
    out << "<Handler " << h.effect << ">\n";
    depth++;
    for(const auto &eImpl : h.implications) {
        visit(eImpl);
    }
    depth--;
}

void ASTPrinter::visit(const TruthLiteral &tl) {
    indent();
    out << "<TruthLiteral " << tl.value << ">\n";
}

void ASTPrinter::visit(const Continuation &k) {
    indent();
    out << "<Continuation>\n";
}

void ASTPrinter::visit(const PredicateDecl &pd) {
    indent();
    out << "<PredicateDecl \"" << pd.name << "\">\n";
    depth++;
    for(const auto &x : pd.parameters) visit(x);
    depth--;
}

void ASTPrinter::visit(const PredicateRef &pr) {
    indent();
    out << "<PredicateRef \"" << pr.name << "\">\n";
    depth++;
    for(const auto &x : pr.arguments) visit(x);
    depth--;
}

void ASTPrinter::visit(const EffectImplHead &eih) {
    indent();
    out << "<EffectCtorRef \"" << eih.effectName << "." << eih.ctorName <<
        "\">\n";
    depth++;
    for(const auto &arg : eih.arguments) {
        visit(arg);
    }
    depth--;
}

void ASTPrinter::visit(const EffectCtorRef &ecr) {
    indent();
    out << "<EffectCtorRef \"" << ecr.effectName << "." << ecr.ctorName <<
        "\">\n";
    depth++;
    for(const auto &arg : ecr.arguments) {
        visit(arg);
    }
    visit(ecr.getContinuation());
    depth--;
}

void ASTPrinter::visit(const Conjunction &conj) {
    indent();
    out << "<Conjunction>\n";
    depth++;
    visit(conj.getLeft());
    visit(conj.getRight());
    depth--;
}

void ASTPrinter::visit(const HandlerConjunction &hConj) {
    indent();
    out << "<HandlerConjunction>\n";
    depth++;
    visit(hConj.getLeft());
    visit(hConj.getRight());
    depth--;
}

void ASTPrinter::visit(const Expression &expr) {
    expr.switchOver(
    [&](TruthLiteral tl) { visit(tl); },
    [&](PredicateRef pr) { visit(pr); },
    [&](EffectCtorRef ecr) { visit(ecr); },
    [&](Conjunction conj) { visit(conj); }
    );
}

void ASTPrinter::visit(const HandlerExpression &hExpr) {
    hExpr.switchOver(
    [&](TruthLiteral tl) { visit(tl); },
    [&](Continuation k) { visit(k); },
    [&](PredicateRef pr) { visit(pr); },
    [&](EffectCtorRef ecr) { visit(ecr); },
    [&](HandlerConjunction hConj) { visit(hConj); }
    );
}

void ASTPrinter::visit(const Implication &impl) {
    indent();
    out << "<Implication>\n";
    depth++;
    visit(impl.head);
    visit(impl.body);
    depth--;
}

void ASTPrinter::visit(const EffectImplication &eImpl) {
    indent();
    out << "<EffectImplication>\n";
    depth++;
    visit(eImpl.head);
    visit(eImpl.body);
    depth--;
}

void ASTPrinter::visit(const AnonymousVariable &av) {
    indent();
    out << "<AnonymousVariable>\n";
}

void ASTPrinter::visit(const Variable &var) {
    indent();
    out << "<Variable \"" << var.name << "\"";
    if(var.isDefinition)
        out << " definition";
    out << ">\n";
}

void ASTPrinter::visit(const StringLiteral &str) {
    indent();
    out << "<StringLiteral \"" << str.value << "\">\n";
}

void ASTPrinter::visit(const IntegerLiteral &i) {
    indent();
    out << "<IntegerLiteral \"" << i.value << "\">\n";
}

void ASTPrinter::visit(const Value &val) {
    val.switchOver(
    [&](AnonymousVariable av) { visit(av); },
    [&](Variable var) { visit(var); },
    [&](ConstructorRef cr) { visit(cr); },
    [&](StringLiteral str) { visit(str); },
    [&](IntegerLiteral i) { visit(i); }
    );
}

void ASTPrinter::visit(const AST &ast) {
    indent();
    out << "<TypedAST>\n";
    depth++;
    for(const auto &x : ast.types) visit(x);
    for(const auto &x : ast.effects) visit(x);
    for(const auto &x : ast.predicates) visit(x);
    depth--;
}

};
