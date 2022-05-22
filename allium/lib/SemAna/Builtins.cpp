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

std::vector<BuiltinPredicate> builtinPredicates = {
    BuiltinPredicate(
        PredicateDecl(
            "concat",
            {
                Parameter("String", true),
                Parameter("String", true),
                Parameter("String", false)
            },
            {}),
        { Mode({true, true, false}, {true, true, true}) })
};

} // end namespace TypedAST
