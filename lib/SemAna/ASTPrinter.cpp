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

void ASTPrinter::visit(const TypeRef &tr) {
    indent();
    out << "<TypeRef \"" << tr << "\">\n";
}

void ASTPrinter::visit(const Predicate &p) {
    indent();
    out << "<Predicate>\n";
    depth++;
    visit(p.declaration);
    for(const auto &x : p.implications) visit(x);
    depth--;
}

void ASTPrinter::visit(const TruthLiteral &tl) {
    indent();
    out << "<TruthLiteral " << tl.value << ">\n";
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

void ASTPrinter::visit(const Conjunction &conj) {
    indent();
    out << "Conjunction>\n";
    depth++;
    visit(conj.getLeft());
    visit(conj.getRight());
    depth--;
}

void ASTPrinter::visit(const Expression &expr) {
    expr.switchOver(
    [&](TruthLiteral tl) { visit(tl); },
    [&](PredicateRef pr) { visit(pr); },
    [&](Conjunction conj) { visit(conj); }
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

void ASTPrinter::visit(const AnonymousVariable &av) {
    indent();
    out << "<AnonymousVariable>\n";
}

void ASTPrinter::visit(const Variable &var) {
    indent();
    out << "<Variable \"" << var.name << "\"";
    if(var.isDefinition)
        out << " definition";
    if(var.isExistential)
        out << " existential";
    out << ">\n";
}

void ASTPrinter::visit(const Value &val) {
    val.switchOver(
    [&](AnonymousVariable av) { visit(av); },
    [&](Variable var) { visit(var); },
    [&](ConstructorRef cr) { visit(cr); }
    );
}

void ASTPrinter::visit(const AST &ast) {
    indent();
    out << "<TypedAST>\n";
    depth++;
    for(const auto &x : ast.types) visit(x);
    for(const auto &x : ast.predicates) visit(x);
    depth--;
}

};
