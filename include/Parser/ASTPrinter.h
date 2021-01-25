#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "Parser/AST.h"

class ASTPrinter {
public:
    ASTPrinter(std::ostream &out): out(out) {}

    void visit(const TruthLiteral &tl) {
        indent();
        out << "<TruthLiteral " << (tl.value ? "true" : "false") <<
            " line:" << tl.location << ">\n";
    }

    void visit(const PredicateDecl &pd) {
        indent();
        out << "<PredicateDecl \"" << pd.name << "\" line:" <<
            pd.location << ">\n";
        ++depth;
        for(const auto &parameter : pd.parameters) {
            visit(parameter);
        }
        --depth;
    }

    void visit(const PredicateRef &pn) {
        indent();
        out << "<PredicateRef \"" << pn.name << "\" line:" <<
            pn.location << ">\n";
        ++depth;
        for(const auto &argument : pn.arguments) {
            visit(argument);
        }
        --depth;
    }

    void visit(const Conjunction &conj) {
        indent();
        out << "<Conjunction>\n";
        ++depth;
        visit(conj.getLeft());
        visit(conj.getRight());
        --depth;
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
        ++depth;
        visit(impl.lhs);
        visit(impl.rhs);
        --depth;
    }
    void visit(const Predicate &p) {
        indent();
        out << "<Predicate \"" << p.name.name << "\" line:" <<
            p.name.location << ">\n";
        ++depth;
        for(const auto &impl : p.implications) {
            visit(impl);
        }
        --depth;
    }

    void visit(const TypeDecl &td) {
        indent();
        out << "<TypeDecl \"" << td.name << "\" line:" <<
            td.location << ">\n";
    }

    void visit(const TypeRef &typeRef) {
        indent();
        out << "<TypeRef \"" << typeRef.name << "\" line:" <<
            typeRef.location << ">\n";
    }

    void visit(const Constructor &ctor) {
        indent();
        out << "<Constructor \"" << ctor.name << "\" line:" <<
            ctor.location << ">\n";
        ++depth;
        for(const auto &param : ctor.parameters) {
            visit(param);
        }
        --depth;
    }

    void visit(const AnonymousVariable &av) {
        indent();
        out << "<AnonymousVariable line:" << av.location << ">\n";
    }

    void visit(const Variable &v) {
        indent();
        out << "<Variable \"" << v.name << "\" line:" << v.location << ">\n";
    }

    void visit(const ConstructorRef &ctor) {
        indent();
        out << "<ConstructorRef \"" << ctor.name << "\" line:" <<
            ctor.location << ">\n";
        ++depth;
        for(const auto &arg : ctor.arguments) {
            visit(arg);
        }
        --depth;
    }

    void visit(const Value &val) {
        val.switchOver(
        [&](AnonymousVariable av) { visit(av); },
        [&](Variable v) { visit(v); },
        [&](ConstructorRef cr) { visit(cr); }
        );
    }

    void visit(const Type &type) {
        out << "<Type \"" << type.declaration.name << "\" line:" <<
            type.declaration.location << ">\n";
        ++depth;
        for(const auto &ctor : type.constructors) {
            visit(ctor);
        }
        --depth;
    }

private:
    void indent() {
        for(int i=0; i<depth; ++i) out << "  ";
    }

    std::ostream &out;
    int depth = 0;
};

static_assert(has_all_visitors<ASTPrinter>());

#endif // AST_PRINTER_H
