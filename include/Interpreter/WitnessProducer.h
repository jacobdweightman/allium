#include "Interpreter/program.h"
#include "values/Generator.h"

namespace interpreter {

bool match(
    const PredicateReference &pr,
    const PredicateReference &matcher,
    std::vector<ConstructorRef> &variables
);

bool match(
    const VariableRef &vr,
    const VariableRef &matcher,
    std::vector<ConstructorRef> &variables
);

bool match(
    const VariableRef &vr,
    const ConstructorRef &cr,
    std::vector<ConstructorRef> &variables
);

bool match(
    const ConstructorRef &cl,
    const ConstructorRef &cr,
    std::vector<ConstructorRef> &variables
);

bool match(
    const Value &left,
    const Value &right,
    std::vector<ConstructorRef> &variables
);

Expression instantiate(
    const Expression &expr,
    const std::vector<ConstructorRef> &variables
);

Generator<std::vector<ConstructorRef> > witnesses(
    const Program &prog,
    const Expression expr
);

Generator<std::vector<ConstructorRef> > witnesses(const TruthValue &tv);

Generator<std::vector<ConstructorRef> > witnesses(
    const Program &prog,
    const PredicateReference &pr
);

Generator<std::vector<ConstructorRef> > witnesses(
    const Program &prog,
    const Conjunction conj
);

} // namespace interpreter
