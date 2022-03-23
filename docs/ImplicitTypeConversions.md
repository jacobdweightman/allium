# Implicit Type Conversions

Suppose that you want to model a parse tree for a context-free grammar. A
straightforward and strongly typed way to do this is to create a type for each
kind of non-terminal symbol in the grammar, with a constructor for each rule
for that symbol, and a parameter for each child in the parse tree. This mirrors
how the Allium parser is designed (in C++) — like the PredicateRef,
Name<Predicate>, and Value types, for example. These typed tree models are
ubiquitous, and are really the “only” kind of data model that Allium supports.

Consider, however, that many grammars have rules where one non-terminal symbol
is rewritten to another (X &rarr; Y). In the presence of these rules, the
programmer is left to define a constructor with a single argument to “wrap” the
sub-parse tree (this is fine), and then forced to explicitly wrap values with
it when constructing values of the type (this is cumbersome).

I stumbled across this recently while writing the boolean satisfiability
solver example. The grammar for these boolean expressions is quite simple:
```
type Bool {
    ctor True;  // <bool> := True
    ctor False; // <bool> := False
}

type Expr {
    ctor B(Bool);         // <expr> := <bool>
    ctor And(Expr, Expr); // <expr> := ( <expr> and <expr> )
    ctor Or(Expr, Expr);  // <expr> := ( <expr> or <expr> )
    ctor Not(Expr);       // <expr> := ( not <expr> )
}
```

Consider now constructing a value to represent "F or (T and F)”:
```
Or(B(False), And(B(True), B(False)))
```

While I don’t think this example is “bad” (at least it’s explicit!), consider
that 9 characters are spent “lifting” Bools into Exprs, which takes up space
and obscures the meaning of the program. I think this is much clearer:
```
Or(False, And(True, False))
```

I propose that this should be made possible. I see a few approaches we might
consider, each with their own tradeoffs.


## Always allow implicit type conversions

When a type `T` has a single 1-ary constructor `Ctor1(U)`, then `U` can be
implicitly converted to `T`. That is, any value of type `U` can be used in
place of an argument of type `T`, so long as the type `T` is inferred. When a
value is implicitly converted to a different type, it is as if it had occurred
inside the constructor explicitly. Note that if there are multiple 1-ary
constructors whose argument has type `U`, then this is ambiguous.

I propose that a type cannot be implicitly converted to itself, since this
could plausibly be an infinite regress — consider Peano’s natural numbers
(allium/tests/Peano.allium):
```
type Nat {
    ctor Zero;
    ctor S(Nat);
}
...
pred p(Nat) {
    p(Zero) <- true;
}
```
If `Nat` could be implicitly converted to `Nat`, then at the very least `Zero`
&rArr; `S(Zero)`. It would be pretty confusing if “zero” in the source program actually meant “one”.

I also propose that implicit conversions should be transitive, so that if `A`
is implicitly convertible to `B` and `B` is implicitly convertible to `C`, then
`A` is implicitly convertible to `C` by composing the constructors. This should
always use the fewest number of conversions, so that there is can never be a cycle.


## Add “explicit” keyword

It probably isn’t always desirable to have implicit conversions happen. As a
somewhat contrived example, consider these types that model even and odd
numbers in Peano arithmetic:
```
type Even {
    ctor Zero;
    ctor S(Odd);
}
type Odd {
    ctor S(Even);
}
pred half(Even, Nat) {
    half(Zero, Zero);
    half(S(S(let x)), S(let y)) <- half(x, y);
}
```
It seems surprising and probably undesirable that one would be able to pass an `Odd` to the `Even` argument of the `half` predicate. This even/odd distinction might have been embedded into the type system to prevent exactly this.

I propose it should be possible to “opt out” of implicit conversions. The constructor could be marked with the “explicit” keyword to prevent it from being used for an implicit conversion. Coincidentally, C++ does exactly this. This would make implicit conversions available by default, but for constructors where this is a bad idea can prevent it.

This has the advantage that programmers will never forget to allow implicit conversions, but the disadvantage that “bad” implicit conversions can happen by default.


## Add “implicit” keyword

Instead of “opting out,” the programmer would “opt in” to implicit conversion by marking the constructor with the “implicit” keyword. This has the advantage that programmers will not see “bad” implicit conversions by default.
