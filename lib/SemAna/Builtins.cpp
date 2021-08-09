#include "SemAna/Builtins.h"

namespace TypedAST {

std::vector<Type> builtinTypes = {
    Type(TypeDecl("Int"), {}),
    Type(TypeDecl("String"), {}),
};

std::vector<Effect> builtinEffects = {
    Effect(
        EffectDecl("IO"),
        {
            EffectCtor("print", { Parameter("String", true) })
        }
    )
};

} // end namespace TypedAST
