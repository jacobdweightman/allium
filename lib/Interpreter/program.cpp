#include <iostream>
#include "Interpreter/program.h"

// looks like hasA(c(a)) is represented as hasA(c(b))...

bool interpreter::Program::prove(const Expression &expr) {
    return expr.match<bool>(
        [](TruthValue tv) -> bool { std::cout << "prove TV: " << tv.value << "\n"; return tv.value; },
        [&](PredicateReference pr) -> bool {
            std::cout << "prove PR: " << pr.index << "\n";
            Predicate p = predicates[pr.index];
            for(Implication implication : p.implications) {
                if(implication.head.matches(pr) && prove(implication.body))
                    return true;
            }
            return false;
        },
        [&](Conjunction conj) -> bool {
            std::cout << "prove conjunction\n";
            return prove(conj.getLeft()) && prove(conj.getRight());
        }
    );
}
 
namespace interpreter {

bool PredicateReference::matches(const PredicateReference &other) const {
    for(int i=0; i<arguments.size(); i++) {
        if(!arguments[i].matches(other.arguments[i]))
            return false;
    }
    return true;
}

bool ConstructorRef::matches(const ConstructorRef &other) const {
    size_t size_t_max = std::numeric_limits<size_t>::max();
    if(index == anonymousVariable || other.index == anonymousVariable) return true;
    if(index != other.index) return false;

    for(int i=0; i<arguments.size(); ++i) {
        if(!arguments[i].matches(other.arguments[i]))
            return false;
    }
    return true;
}

} // namespace interpreter
