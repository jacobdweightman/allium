#ifndef TYPED_AST_PRINTER_H
#define TYPED_AST_PRINTER_H

#include <ostream>
#include "TypedAST.h"

namespace TypedAST {

class ASTPrinter {
public:
    ASTPrinter(std::ostream &out): out(out) {}

    void visit(const TypeDecl &td) {
        indent();
        out << "<TypeDecl \"" << td.name << "\">\n";
    }

    void visit(const Constructor &ctor) {
        indent();
        out << "<Constructor \"" << ctor.name << "\">\n";
        depth++;
        for(const auto &x : ctor.parameters) visit(x);
        depth--;
    }

    void visit(const ConstructorRef &cr) {
        indent();
        out << "<ConstructorRef \"" << cr.name << "\">\n";
        depth++;
        for(const auto &x : cr.arguments) visit(x);
        depth--;
    }

    void visit(const Type &type) {
        indent();
        out << "<Type>\n";
        depth++;
        visit(type.declaration);
        for(const auto &x : type.constructors) visit(x);
        depth--;
    }

    void visit(const TypeRef &tr) {
        indent();
        out << "<TypeRef \"" << tr << "\">\n";
    }

    void visit(const Predicate &p) {
        indent();
        out << "<Predicate>\n";
        depth++;
        visit(p.declaration);
        for(const auto &x : p.implications) visit(x);
        depth--;
    }

    void visit(const TruthLiteral &tl) {
        indent();
        out << "<TruthLiteral " << tl.value << ">\n";
    }

    void visit(const PredicateDecl &pd) {
        indent();
        out << "<PredicateDecl \"" << pd.name << "\">\n";
        depth++;
        for(const auto &x : pd.parameters) visit(x);
        depth--;
    }

    void visit(const PredicateRef &pr) {
        indent();
        out << "<PredicateRef \"" << pr.name << ">\n";
        depth++;
        for(const auto &x : pr.arguments) visit(x);
        depth--;
    }

    void visit(const Conjunction &conj) {
        indent();
        out << "Conjunction>\n";
        depth++;
        visit(conj.getLeft());
        visit(conj.getRight());
        depth--;
    }

    void visit(const Expression &expr) {
        expr.switchOver(
        [&](TruthLiteral tl) { visit(tl); },
        [&](PredicateRef pr) { visit(pr); },
        [&](Conjunction conj) { visit(conj); }
        );
    }

    void visit(const Implication &impl) {
        indent();
        out << "<Implication>\n";
        depth++;
        visit(impl.head);
        visit(impl.body);
        depth--;
    }

    void visit(const AnonymousVariable &av) {
        indent();
        out << "<AnonymousVariable>\n";
    }

    void visit(const Variable &var) {
        indent();
        if(var.isDefinition) {
             out << "Variable \"" << var.name << "\" definition>\n";
        } else {
            out << "Variable \"" << var.name << "\">\n";
        }
    }

    void visit(const Value &val) {
        val.switchOver(
        [&](AnonymousVariable av) { visit(av); },
        [&](Variable var) { visit(var); },
        [&](ConstructorRef cr) { visit(cr); }
        );
    }

    void visit(const AST &ast) {
        indent();
        out << "<TypedAST>\n";
        depth++;
        for(const auto &x : ast.types) visit(x);
        for(const auto &x : ast.predicates) visit(x);
        depth--;
    }

private:
    void indent() {
        for(int i=0; i<depth; ++i) out << "  ";
    }

    std::ostream &out;
    int depth = 0;
};

};

#endif // TYPED_AST_PRINTER_H
