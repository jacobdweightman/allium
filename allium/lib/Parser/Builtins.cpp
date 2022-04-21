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

} // end namespace parser
