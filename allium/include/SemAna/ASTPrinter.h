#ifndef TYPED_AST_PRINTER_H
#define TYPED_AST_PRINTER_H

#include <ostream>

#include "TypedAST.h"

namespace TypedAST {

class ASTPrinter {
public:
    ASTPrinter(std::ostream &out): out(out) {}

    void visit(const TypeDecl &td);
    void visit(const CtorParameter &tr);
    void visit(const Constructor &ctor);
    void visit(const ConstructorRef &cr);
    void visit(const Type &type);
    void visit(const EffectDecl &eDecl);
    void visit(const Parameter &p);
    void visit(const EffectCtor &eCtor);
    void visit(const Effect &e);
    void visit(const UserPredicate &up);
    void visit(const Handler &h);
    void visit(const TruthLiteral &tl);
    void visit(const Continuation &k);
    void visit(const PredicateDecl &pd);
    void visit(const PredicateRef &pr);
    void visit(const EffectImplHead &eih);
    void visit(const EffectCtorRef &ecr);
    void visit(const Conjunction &conj);
    void visit(const HandlerConjunction &hConj);
    void visit(const Expression &expr);
    void visit(const HandlerExpression &hExpr);
    void visit(const Implication &impl);
    void visit(const EffectImplication &eImpl);
    void visit(const AnonymousVariable &av);
    void visit(const Variable &var);
    void visit(const StringLiteral &str);
    void visit(const IntegerLiteral &i);
    void visit(const Value &val);
    void visit(const AST &ast);

private:
    void indent() {
        for(int i=0; i<depth; ++i) out << "  ";
    }

    std::ostream &out;
    int depth = 0;
};

};

#endif // TYPED_AST_PRINTER_H
