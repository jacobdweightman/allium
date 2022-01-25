#include <assert.h>
#include <map>
#include <vector>

#include "SemAna/GroundAnalysis.h"
#include "SemAna/PredRecursionAnalysis.h"
#include "SemAna/VariableAnalysis.h"
#include "Utils/TaggedUnion.h"
#include "Utils/VectorUtils.h"

/*
 * This analysis analyzes an entire program to ensure that parameters with the
 * "in" modifier are respected. The values passed to arguments with the "in"
 * modifier must be a ground value, i.e. not contain unbound variables.
 *
 * A value is ground iff it is a:
 *  - literal
 *  - constructor, and all of its arguments (possibly all 0 of them) are also
 *    ground
 *  - variable which has been unified with a ground value
 * 
 * For this analysis to be useful, we need to determine whether a variable which
 * is passed as an argument to a predicate is ground. Because the groundness of
 * a variable changes during execution, the execution model matters. This 
 * analysis assumes all sub-proofs start with pattern matching proceed in a
 * left-to-right, depth-first order. A variable is ground if it is ground in all
 * of a predicate's witnesses.
 * 
 * Whether or not a predicate produces a witness is undecidable since Allium is
 * Turing complete. However, if Allium cannot construct a finite proof or
 * disprove (by exhaustion, using the closed world assumption), then it doesn't
 * matter whether a variable in its arguments is ground or not because it
 * cannot subsequently be used. Thus, we assume that we find a proof in a finite
 * number of steps, and so it is possible to prove a variable's groundness by
 * induction on the number of proof steps.
 */

namespace TypedAST {

typedef std::map<Name<Variable>, bool> Context;

class ValueGroundness;

struct CtorGroundness {
    Name<Constructor> ctor;
    std::vector<ValueGroundness> arguments;

    bool operator==(const CtorGroundness &other) const;
    bool operator<(const CtorGroundness &other) const;

    CtorGroundness(Name<Constructor> ctor, std::vector<ValueGroundness> arguments):
        ctor(ctor), arguments(arguments) {}
};

typedef TaggedUnion<bool, CtorGroundness> ValueGroundnessBase;
class ValueGroundness : public ValueGroundnessBase {
public:
    using ValueGroundnessBase::ValueGroundnessBase;

    bool operator<(const ValueGroundness &other) const;

    ValueGroundness& operator&=(const ValueGroundness &other);

    bool isGround() const;
};

bool CtorGroundness::operator==(const CtorGroundness &other) const {
    return ctor == other.ctor && arguments == other.arguments;
}

bool CtorGroundness::operator<(const CtorGroundness &other) const {
    if(arguments.size() != other.arguments.size())
        return arguments.size() < other.arguments.size();
    if(ctor != other.ctor)
        return ctor < other.ctor;
    for(int i=0; i<arguments.size(); ++i) {
        if(arguments[i] != other.arguments[i])
            return arguments[i] < other.arguments[i];
    }
    return false;
}

bool ValueGroundness::operator<(const ValueGroundness &other) const {
    return match<bool>(
    [&](bool lb) {
        return other.match<bool>(
        [&](bool rb) {
            return lb < rb;
        },
        [&](const CtorGroundness &) {
            return true;
        });
    },
    [&](CtorGroundness &lcg) {
        return other.match<bool>(
        [&](bool) {
            return false;
        },
        [&](const CtorGroundness &rcg) {
            return lcg < rcg;
        });
    });
}

ValueGroundness& ValueGroundness::operator&=(const ValueGroundness &other) {
    switchOver(
    [&](bool groundness) {
        if(groundness)
            *this = other;
    },
    [&](CtorGroundness &cg) {
        other.switchOver(
        [&](bool groundness) {
            if(!groundness)
                *this = other;
        },
        [&](const CtorGroundness &cg2) {
            for(size_t i=0; i<cg.arguments.size(); ++i) {
                cg.arguments[i] &= cg2.arguments[i];
            }
        });
    });
    return *this;
}

bool ValueGroundness::isGround() const {
    return match<bool>(
    [](bool groundness) { return groundness; },
    [](const CtorGroundness &cg) {
        for(const ValueGroundness &vg : cg.arguments) {
            if(!vg.isGround())
                return false;
        }
        return true;
    });
}

struct PRGroundness {
    Name<Predicate> name;
    std::vector<ValueGroundness> arguments;

    PRGroundness(): name(""), arguments({}) {}

    PRGroundness(Name<Predicate> name, std::vector<ValueGroundness> arguments):
        name(name), arguments(arguments) {}

    bool operator<(const PRGroundness &other) const {
        if(arguments.size() != other.arguments.size())
            return arguments.size() < other.arguments.size();
        if(name != other.name)
            return name < other.name;
        for(int i=0; i<arguments.size(); ++i) {
            if(arguments[i] != other.arguments[i])
                return arguments[i] < other.arguments[i];
        }
        return false;
    }

    PRGroundness& operator&=(const PRGroundness &other) {
        assert(name == other.name);
        assert(arguments.size() == other.arguments.size());

        for(size_t i=0; i<arguments.size(); ++i) {
            arguments[i] &= other.arguments[i];
        }
        return *this;
    }
};

std::ostream& operator<<(std::ostream &out, const ValueGroundness &vg) {
    vg.switchOver(
    [&](bool groundness) {
        out << (groundness ? "ground" : "nonground");
    },
    [&](const CtorGroundness &cg) {
        out << cg.ctor << "(";
        for(const auto &arg : cg.arguments) {
            out << arg;
            if(arg != cg.arguments.back())
                out << ", ";
        }
        out << ")";
    });
    return out;
}

std::ostream& operator<<(std::ostream &out, const PRGroundness &prg) {
    out << prg.name << "(";
    for(const auto &arg : prg.arguments) {
        out << arg;
        if(arg != prg.arguments.back())
            out << ", ";
    }
    return out << ")";
}

class GroundAnalysis {
    const AST &ast;
    ErrorEmitter &error;

    // Analysis results
    const PredDependenceGraph pdg;

    // A memo which records a mapping of "input" groundness to "output" groundness
    // for predicate modes which have already been (partially) computed. This is
    // refined as the analysis proceeds for recursive predicates.
    std::map<PRGroundness, PRGroundness> memo;

    /// This is a hack to easily pass around the name of a troublesome variable
    /// for diagnostic messages.
    /// TODO: fold this into `isGround` if that method isn't used anywhere else.
    Optional<std::string> uninstantiatedVariableName;

    void emitGroundingError(SourceLocation location) {
        uninstantiatedVariableName.switchOver<void>(
        [&](std::string uninstantiatedVariableName) -> void {
            error.emit(
                location,
                ErrorMessage::argument_is_not_ground,
                uninstantiatedVariableName);
        },
        [&]() -> void {
            error.emit(
                location,
                ErrorMessage::argument_is_not_ground_anonymous);
        });
    }

    /// True iff val contains no anonymous or unbound variables in the given context.
    bool isGround(const Context &ctx, const Value &val) {
        return val.match<bool>(
        [&](AnonymousVariable &) {
            uninstantiatedVariableName = Optional<std::string>();
            return false;
        },
        [&](Variable &v) {
            bool result = ctx.at(v.name);
            if(!result)
                uninstantiatedVariableName = v.name.string();
            return result;
        },
        [&](ConstructorRef &cr) {
            for(const Value &arg : cr.arguments) {
                if(!isGround(ctx, arg)) {
                    return false;
                }
            }
            return true;
        },
        [](StringLiteral &) { return true; },
        [](IntegerLiteral &) { return true; }
        );
    }

    /// Marks any variables in `val` as ground in `ctx`. Returns whether
    /// or not any change occurred.
    bool groundAllVariables(Context &ctx, const Value &val) {
        // This can be a function instead of a method...
        return val.match<bool>(
        [](AnonymousVariable &) { return false; },
        [&](Variable &v) {
            if(!ctx.at(v.name)) {
                // std::cout << v.name << " is ground.\n";
                ctx.at(v.name) = true;
                return true;
            }
            return false;
        },
        [&](ConstructorRef &cr) {
            bool changed = false;
            for(const auto &arg : cr.arguments) {
                changed |= groundAllVariables(ctx, arg);
            }
            return changed;
        },
        [](StringLiteral &) { return false; },
        [](IntegerLiteral &) { return false; }
        );
    }

    bool groundVariablesSmart(
        Context &ctx1, const Value &v1,
        Context &ctx2, const Value &v2
    ) {
        if(isGround(ctx1, v1))
            return groundAllVariables(ctx2, v2);
        if(isGround(ctx2, v2))
            return groundAllVariables(ctx1, v1);

        return v1.match<bool>(
        [](AnonymousVariable &) { return false; },
        [&](Variable &var1) {
            // v2 must not be ground, otherwise we would have already returned.
            return false;
        },
        [&](ConstructorRef &cr1) {
            return v2.match<bool>(
            [](AnonymousVariable &) { return false; },
            [&](Variable &var2) {
                // v1 must not be ground, otherwise we would have already returned.
                return false;
            },
            [&](ConstructorRef &cr2) {
                bool changed = false;
                for(int i=0; i<cr1.arguments.size(); ++i) {
                    changed |= groundVariablesSmart(
                        ctx1, cr1.arguments[i],
                        ctx2, cr2.arguments[i]);
                }
                return changed;
            },
            [](StringLiteral &) {
                assert(false && "unreachable: v2 is ground!");
                return false;
            },
            [](IntegerLiteral &) {
                assert(false && "unreachable: v2 is ground!");
                return false;
            });
        },
        [](StringLiteral &) {
            assert(false && "unreachable: v1 is ground!");
            return false;
        },
        [](IntegerLiteral &) {
            assert(false && "unreachable: v1 is ground!");
            return false;
        });

    }

    ValueGroundness getGroundness(const Context &ctx, const Value &val) {
        // TODO: optimize by eliminating redundant groundness checking
        if(isGround(ctx, val))
            return true;
        
        return val.match<ValueGroundness>(
        [](const AnonymousVariable&) { return false; },
        [&](const Variable &var) { return ctx.at(var.name); },
        [&](ConstructorRef &cr) {
            std::vector<ValueGroundness> argGroundness;
            bool allGround = true;
            for(const auto &arg : cr.arguments) {
                ValueGroundness ag = getGroundness(ctx, arg);
                allGround &= (ag == true);
                argGroundness.push_back(ag);
            }

            if(allGround) {
                return ValueGroundness(true);
            } else {
                return ValueGroundness(CtorGroundness(cr.name, argGroundness));
            }
        },
        [](const StringLiteral &) { return true; },
        [](const IntegerLiteral &) { return true; }
        );
    }

    PRGroundness getGroundness(const Context &ctx, const PredicateRef &pr) {
        return PRGroundness(
            pr.name,
            map<Value, ValueGroundness>(
                pr.arguments,
                [&](Value v) { return getGroundness(ctx, v); }));
    }

    /// True iff a new argument was proven to always be ground.
    bool analyzePredicateRef(Context &ctx, const PredicateRef &pr) {
        // std::cout << "analyze: " << pr << std::endl;

        const Predicate &p = ast.resolvePredicateRef(pr);

        // The groundness of the predicate arguments before the subproof.
        PRGroundness initialGroundness = getGroundness(ctx, pr);

        // std::cout << "initial groundness: " << initialGroundness << "\n";

        // memoization
        auto memoizedResult = memo.find(initialGroundness);
        if(memoizedResult != memo.end()) {
            // std::cout << "memo hit: " << memoizedResult->second << "\n";
            bool changed = false;
            for(int i=0; i<pr.arguments.size(); ++i) {
                if(memoizedResult->second.arguments[i].isGround()) {
                    changed |= groundAllVariables(ctx, pr.arguments[i]);
                }
            }
            return changed;
        }

        // True if an argument should be marked ground _after_ the subproof.
        PRGroundness finalGroundness(
            pr.name,
            std::vector<ValueGroundness>(pr.arguments.size(), true));

        auto [nonrecursiveImpls, recursiveImpls] = partitionRecursiveImpls(p);

        // A predicate's implications which cannot make a recursive call produce
        // proofs in the fewest steps. If a variable is ground for all of these,
        // this forms a base case for an inductive proof that it is always ground.
        // std::cout << "base case\n";
        for(const auto &impl : nonrecursiveImpls) {
            analyzeImpl(ctx, pr, impl, finalGroundness);
        }

        // insert the "base case" result into the memo.
        // std::cout << "base conclusion: " << finalGroundness << "\n";
        memo[initialGroundness] = finalGroundness;

        // For implications which may make a recursive call, we rely on the memo
        // to provide the right induction hypothesis. If a variable is also ground
        // for all recursive implications, then it is always ground. (Since we assume
        // the proof succeeds, this should be valid by induction on proof length.)
        for(const auto &impl : recursiveImpls) {
            analyzeImpl(ctx, pr, impl, finalGroundness);
        }

        bool changed = false;

        // If the proof of the predicate succeeds, and the argument is ground in
        // every implication for that predicate, then it is ground.
        for(int i=0; i<pr.arguments.size(); ++i) {
            if(finalGroundness.arguments[i].isGround()) {
                groundAllVariables(ctx, pr.arguments[i]);
                changed = true;
            } else {
                memo[initialGroundness].arguments[i] = false;
            }
        }

        return changed;
    }

    std::pair<std::vector<Implication>, std::vector<Implication>>
    partitionRecursiveImpls(const Predicate &p) {
        std::vector<Implication> nonrecursiveImpls, recursiveImpls;

        for(const auto &impl : p.implications) {
            bool implIsRecursive;
            forAllPredRefs(impl.body, [&](const PredicateRef &pr) {
                implIsRecursive |= pdg.dependsOn(pr.name, p.declaration.name);
            });
            if(implIsRecursive) {
                recursiveImpls.push_back(impl);
            } else {
                nonrecursiveImpls.push_back(impl);
            }
        }

        return std::make_pair(nonrecursiveImpls, recursiveImpls);
    }

    void analyzeImpl(
        Context &ctx,
        const PredicateRef &pr,
        const Implication &impl,
        PRGroundness &shouldGround
    ) {
        // std::cout << "next impl\n";
        Context innerCtx;
        for(const auto &variable : getVariables(impl)) {
            innerCtx.insert({ variable, false });
        }

        // Propagate groundness from caller to head
        for(int i=0; i<pr.arguments.size(); ++i) {
            groundVariablesSmart(
                ctx, pr.arguments[i],
                innerCtx, impl.head.arguments[i]
            );
        }

        // Propagate groundness through body
        while(analyzeExpression(innerCtx, impl.body));

        // Propagate groundness back to caller
        for(int i=0; i<pr.arguments.size(); ++i) {
            shouldGround.arguments[i] &=
                getGroundness(innerCtx, impl.head.arguments[i]);
        }
    }

    bool analyzeEffectCtorRef(Context &ctx, const EffectCtorRef &ecr) {
        // TODO: once handlers are supported, we will need to resolve them
        // in order to see if their arguments are provably ground.
        const auto &parameters = ast.resolveEffectCtorRef(ecr).parameters;
        for(int i=0; i<parameters.size(); ++i) {
            if(parameters[i].isInputOnly && !isGround(ctx, ecr.arguments[i])) {
                emitGroundingError(ecr.location);
            }
        }
        return false;
    }

    bool analyzeExpression(Context &ctx, const Expression &expr) {
        return expr.match<bool>(
        [](TruthLiteral &) { return false; },
        [&](PredicateRef &pr) { return analyzePredicateRef(ctx, pr); },
        [&](EffectCtorRef &ecr) { return analyzeEffectCtorRef(ctx, ecr); },
        [&](Conjunction & conj) {
            bool leftChanged = analyzeExpression(ctx, conj.getLeft());
            bool rightChanged = analyzeExpression(ctx, conj.getRight());
            return leftChanged || rightChanged;
        });
    }

public:
    GroundAnalysis(const AST &ast, ErrorEmitter &error):
        ast(ast), error(error), pdg(ast) {}

    void analyzeMain() {
        // TODO: Currently main is the only possible entry point to an Allium
        // program. When we eventually support libraries, we will need to decide
        // on a way to specify "modes" for public predicates.
        Context ctx;
        analyzePredicateRef(ctx, PredicateRef("main", {}));
    }
};

void checkGroundParameters(const AST &ast, ErrorEmitter &error) {
    GroundAnalysis(ast, error).analyzeMain();
}

}
