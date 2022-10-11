#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "Parser/AST.h"

namespace parser {

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
        for(const auto &e : pd.effects) {
            visit(e);
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

    void visit(const EffectImplHead &eih) {
        indent();
        out << "<EffectImplHead \"" << eih.name << "\" line:" << eih.location <<
            ">\n";
        ++depth;
        for(const auto &argument : eih.arguments) {
            visit(argument);
        }
        --depth;
    }

    void visit(const EffectCtorRef &ecr) {
        indent();
        out << "<EffectCtorRef \"" << ecr.name << "\" line:" << ecr.location <<
            ">\n";
        ++depth;
        for(const auto &argument : ecr.arguments) {
            visit(argument);
        }
        visit(ecr.getContinuation());
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
        [&](EffectCtorRef ecr) { visit(ecr); },
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
        out << "<Predicate>\n";
        ++depth;
        visit(p.name);
        for(const auto &impl : p.implications) visit(impl);
        for(const auto &handler : p.handlers) visit(handler);
        --depth;
    }

    void visit(const TypeDecl &td) {
        indent();
        out << "<TypeDecl \"" << td.name << "\" line:" <<
            td.location << ">\n";
    }

    void visit(const CtorParameter &cp) {
        indent();
        out << "<CtorParameter \"" << cp.name << "\" line:" <<
            cp.location << ">\n";
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

    void visit(const NamedValue &val) {
        indent();
        if(val.isDefinition) {
            out << "<Value definition \"" << val.name << "\" line:" << val.location << ">\n";
        } else {
            out << "<Value \"" << val.name << "\" line:" << val.location << ">\n";
            ++depth;
            for(const auto &arg : val.arguments) {
                visit(arg);
            }
            --depth;
        }
    }

    void visit(const StringLiteral &str) {
        indent();
        out << "<StringLiteral \"" << str.text << "\" line:" << str.location << ">\n";
    }

    void visit(const IntegerLiteral &i) {
        indent();
        out << "<IntegerLiteral " << i.value << " line:" << i.location << ">\n";
    }

    void visit(const Value &val) {
        val.switchOver(
        [&](NamedValue nv) { visit(nv); },
        [&](StringLiteral str) { visit(str); },
        [&](IntegerLiteral i) { visit(i); }
        );
    }

    void visit(const Type &type) {
        indent();
        out << "<Type>\n";
        ++depth;
        visit(type.declaration);
        for(const auto &ctor : type.constructors) {
            visit(ctor);
        }
        --depth;
    }

    void visit(const EffectRef &e) {
        indent();
        out << "<EffectRef \"" << e.name << "\" line:" << e.location << ">\n";
    }

    void visit(const EffectDecl &decl) {
        indent();
        out << "<EffectDecl \"" << decl.name << "\" line:" <<
            decl.location << ">\n";
    }

    void visit(const Parameter &p) {
        indent();
        out << "<Parameter \"" << p.name << "\"" <<
            (p.isInputOnly ? " in" : "") << " line:" << p.location << ">\n";
    }

    void visit(const EffectConstructor &ctor) {
        indent();
        out << "<EffectConstructor \"" << ctor.name << "\" line:" <<
            ctor.location << ">\n";
        ++depth;
        for(const auto &param : ctor.parameters) {
            visit(param);
        }
        --depth;
    }

    void visit(const Effect &effect) {
        indent();
        out << "<Effect>\n";
        ++depth;
        visit(effect.declaration);
        for(const auto &ctor : effect.constructors) {
            visit(ctor);
        }
        --depth;
    }

    void visit(const Handler &handler) {
        indent();
        out << "<Handler>\n";
        ++depth;
        visit(handler.effect);
        for(const auto &effectImplication : handler.implications) {
            visit(effectImplication);
        }
        --depth;
    }

    void visit(const EffectImplication &effectImplication) {
        indent();
        out << "<EffectImplication>\n";
        ++depth;
        visit(effectImplication.head);
        visit(effectImplication.body);
        --depth;
    }

    void visit(const AST &ast) {
        indent();
        out << "<AST>\n";
        ++depth;
        for(const auto &type : ast.types) visit(type);
        for(const auto &effect : ast.effects) visit(effect);
        for(const auto &predicate : ast.predicates) visit(predicate);
        --depth;
    }

private:
    void indent() {
        for(int i=0; i<depth; ++i) out << "  ";
    }

    std::ostream &out;
    int depth = 0;
};

static_assert(has_all_visitors<ASTPrinter>(), "missing visitor(s).");

} // namespace parser

#endif // AST_PRINTER_H
