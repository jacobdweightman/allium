#include "Parser/Builtins.h"

namespace parser {

std::set<Name<Type>> builtinTypes = {
    "Int",
    "String"
};

bool nameIsBuiltinType(Name<parser::Type> tn) {
    return builtinTypes.contains(tn);
}

std::vector<Effect> builtinEffects = {
    Effect(
        EffectDecl("IO", SourceLocation()),
        {
            EffectConstructor(
                "print",
                { Parameter("String", true, SourceLocation()) },
                SourceLocation()
            )
        }
    )
};

std::vector<PredicateDecl> builtinPredicates = {
    PredicateDecl(
        "concat",
        {
            Parameter("String", true, SourceLocation()),
            Parameter("String", true, SourceLocation()),
            Parameter("String", false, SourceLocation())
        },
        {},
        {})
};

} // end namespace parser
