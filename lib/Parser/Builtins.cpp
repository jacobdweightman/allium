#include "Parser/Builtins.h"

namespace parser {

std::vector<Type> builtinTypes = {
    Type(TypeDecl("Int", SourceLocation()), {}),
    Type(TypeDecl("String", SourceLocation()), {}),
};

Optional<Type *> getTypeIfBuiltin(Name<parser::Type> tn) {
    auto match = std::find_if(
        builtinTypes.begin(),
        builtinTypes.end(),
        [&](Type &t) { return tn.string() == t.declaration.name.string(); });
    
    if(match == builtinTypes.end()) {
        return Optional<Type *>();
    } else {
        return &(*match);
    }
}

bool nameIsBuiltinType(Name<parser::Type> tn) {
    return getTypeIfBuiltin(tn).operator bool();
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
