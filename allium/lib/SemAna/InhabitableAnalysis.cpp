#include "SemAna/InhabitableAnalysis.h"
#include "Utils/VectorUtils.h"

namespace TypedAST {

std::set<Name<Type>> getInhabitableTypes(const std::vector<Type> &types) {
    /*
     * Assume all types are uninhabited until proven otherwise. Then, attempt to
     * prove types are inhabited until a fixpoint is reached:
     *  1. Literal types are inhabited
     *  2. If a type has a constructor with all arguments of inhabited types, it
     *     is also inhabited.
     *
     * The outer loop runs at most N times because at least one type is proven
     * inhabitable on each iteration (otherwise a fixpoint was already reached).
     * The inner loop runs at most N times (once for each type). Assuming the
     * number of constructors per type is independent of the number of types,
     * this algorithm has worst-case complexity O(N^2).
     */
    std::set<const Type*> uninhabitedTypes;
    std::set<Name<Type>> inhabitedTypes;

    for(const auto &type : types) {
        uninhabitedTypes.insert(&type);
    }

    // Literal types are inhabited, even if they have no constructors. They
    // are also never user-defined types.
    inhabitedTypes.insert(Name<Type>("Int"));
    inhabitedTypes.insert(Name<Type>("String"));

    bool changed;
    do {
        changed = false;
        auto type = uninhabitedTypes.begin();
        while(type != uninhabitedTypes.end()) {
            bool typeIsInhabited = false;

            // A type is inhabited if it has a constructor whose arguments are
            // all of inhabited types.
            for(const auto &ctor : (*type)->constructors) {
                bool allArgumentsAreInhabited = true;
                for(const auto &param : ctor.parameters) {
                    if(!inhabitedTypes.contains(param.type)) {
                        allArgumentsAreInhabited = false;
                        break;
                    }
                }
                if(allArgumentsAreInhabited) {
                    inhabitedTypes.insert((*type)->declaration.name);
                    type = uninhabitedTypes.erase(type);
                    typeIsInhabited = true;
                    changed = true;
                    break;
                }
            }

            // If the type was erased from the set of uninhabited types, then
            // erasing already moves the iterator to the next type.
            if(!typeIsInhabited) {
                ++type;
            }
        }
    } while(changed);

    return inhabitedTypes;
}

}
