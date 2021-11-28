# The Allium Programming Language

This page documents the syntax and semantics of Allium.

## Program Structure

The basic building blocks of Allium programs are predicates, which are
representations of predicates in first-order logic. Fundamentally, an Allium
program is a procedure for finding a proof of a special predicate called main.

Allium predicates are defined using the keyword `pred`, and must be given a
name; each predicate also has an _exhaustive_ list of implications which can be
used to prove it. Each of a predicate's implications must have the predicate as
its head — that is, each of a predicate's implications must actually provide a
way to prove that predicate.

Thus, the simplest possible Allium program consists of a single predicate called
`main`, which has no implications. Because there are no implications, there is
no way to prove `main`, and so this program always fails.

```
pred main {}
```

## Logical Expressions

In order to write more interesting programs, we need language terms that
correspond to more interesting logical constructs.

The simplest among these are the truth value literals. The Allium keyword
`true` denotes a proposition which is trivial to prove. The keyword `false`
denotes a proposition which cannot be proven.

Now that we can write down a logical expression, it is possible to introduce the
syntax for an implication. Often in formal logic, a statement like "if P then Q"
is written as

![P -> Q](https://latex.codecogs.com/gif.image?%5Cdpi%7B110%7D%20%5Cbg_white%20P%20%5Crightarrow%20Q%20)

For syntactic convenience, it is preferrable to "flip this horizontally" in
Allium, such that the "then" part (called the "head") comes first. Thus, for a
predicate `Q` and a logical expression `P`, we can write the implication as
```
Q <- P;
```

For example, tt is possible to write a program which always succeeds, using the
truth value `true`:
```
pred main {
    main <- true;
}
```

Logical expressions can also be terms which correspond to defined predicates. A
predicate can be proven using its implications. The only method of proving
predicates in Allium is _modus ponens_:

```
pred p {
    // true is trivial, therefore p.
    p <- true;
}

pred main {
    // By definition, p -> main.
    main <- p;
}

// proof:
// p, p -> main
// ------------
// main
```

The final form of logical expressions is as a conjunction (logical AND) of
other logical expressions. In a program, conjunction is written as two logical
expressions separated by a comma (`,`). The only method of proving a
conjunction in Allium is by proving each of the conjuncts and using conjunction
introduction:
```
pred p {
    p <- true;
}

pred q {
    q <- true;
}

pred main {
    main <- p, q;
}

// proof:
// p, q
// ------
// p /\ q
//
// p /\ q, p /\ q -> main
// ----------------------
// main
```

At this point, you may be wondering about the Allium syntax for disjunction
(logical OR). There is no such syntax, nor is there currently any plan to add it.
It's possible to achieve disjunction by using multiple implications:

```
pred p {}

pred q {
    q <- true;
}

pred main {
    main <- p;
    main <- q;
}
```

Because `main` has _two_ implications, there are _two_ ways to prove it: the
first is to prove `p`, which happens to be impossible; the second is to prove
`q`, which is trivial. Focussing on just the definition of `main`, if `p` or `q`,
then `main` follows.

Recall that an Allium predicate's list of implications is defined to be
exhaustive — that is, there are no other ways to prove a predicate besides those
in the program. This is sometimes called the "closed world assumption," and it
means our list of implications actually gives us a complete definition of the
predicate: a predicate is true if and only if at least one of its implications
is true. In the previous example, this translates into

![foo](https://latex.codecogs.com/gif.image?%5Cdpi%7B200%7D%20%5Cbg_white%20%5Ctexttt%7Bmain%7D%20%5Ciff%20%5Ctexttt%7Bp%7D%20%5Cvee%20%5Ctexttt%7Bq%7D)

By expressing disjunction in this way, it's possible to write Allium programs
which construct proofs of propositional logic using only _modus ponens_ and
conjunction introduction, and specifically _without_ disjunction elimination
without losing any power — the disjuctions simply need to be eliminated in
advance.

## Representing Data

The previous section showed how Allium programs correspond to proofs in
propositional logic, but this is of limited use for solving practical problems.
Real problems work with data, and useful procedures have inputs. Thus, it will
be useful to _parameterize_ our predicates. We first discuss what these _values_ are allowed to be.

Types are a central part of Allium. A type has a fixed set of constructors, and
every value belonging to that type is defined with exactly one of its
constructors. For example, if our program needs to represent the flavors of ice
cream at an ice cream shop, each flavor could have its own constructor. This is
closely analogous to enumerated types in other languages:

```
type IceCreamFlavor {
    ctor Vanilla;
    ctor Chocolate;
    ctor Strawberry;
}

type Sauce {
    ctor Fudge;
    ctor Caramel;
    ctor Raspberry;
}
```

Later on in the program, anywhere that a value of type `IceCreamFlavor` is
expected, one could use any of the values `Vanilla`, `Chocolate`, or
`Strawberry`. Anywhere that a value of type `Sauce` is expected, one could use
any of the values `Fudge`, `Caramel`, or `Raspberry`. however, Allium does not
allow one to use `Caramel` in place of an `IceCreamFlavor`, because that's a
`Sauce` and not an `IceCreamFlavor`.

Constructors can also take arguments of fixed types, which can be used to define
types similar to structures in other languages:

```
type Sundae {
    ctor Sundae(IceCreamFlavor, Sauce);
}
```

Later on, one could make a value of type `Sundae` using an expression like
`Sundae(Vanilla, Raspberry)`, or using the customer's choice of ice cream and
sauce.

Lastly, types are allowed to be recursive. For example, a customer's order might
be a list of sundaes, which we can represent as follows:

```
type SundaeList {
    ctor Nil;
    ctor Cons(Sundae, SundaeList);
}
```

Thus, a family might walk into the store and place this order:
```
Cons(Sundae(Chocolate, Fudge),
    Cons(Sundae(Vanilla, Caramel),
        Cons(Sundae(Strawberry, Raspberry))))
```

Someday, Allium will support generic types and provide a `List` type in the
standard library, so that an order would be of type `List<Sundae>`. There will
also be a special syntax for lists using square braces! 

## Arguments

Now that we know how to define types and construct values, we can add parameters
to our predicates. The number, types, and order of predicate parameters are
defined in the predicate declaration using a comma-separated list of type names
enclosed in parentheses after the predicate name. For example, if we wanted to
define a predicate called `containsChocolate` which is true if and only if any
of the ingredients contain chocolate, we would declare it like this:

```
pred containsChocolate(Sundae) {
    ...
}
```

When writing implications for a predicate with arguments, it is usually helpful
to make use of _pattern matching_. When Allium attempts to prove a predicate,
it will examine each of its implications and attempt to _match_ that predicate
with its head. Only implications whose heads match the predicate can be used to prove it. The rules for this matching are as follows:
1. the predicates match if all of their arguments match;
2. constructors match if they are the same constructor and all of their 
   (possibly zero) arguments match;
3. the special symbol `_` matches anything;
4. variables being defined match against anything, and the variable binds to
   whatever it matches;
5. variables which were previously defined match if the value to which it is
   bound matches.

For example, any sundae with chocolate ice cream contains chocolate, and similarly any sundae with fudge sauce also contains chocolate. We can codify this with the following implications:

```
containsChocolate(Sundae(Chocolate, _)) <- true;
containsChocolate(Sundae(_, Fudge)) <- true;
```

## Variables

The previous discussion on pattern matching mentioned variables, but didn't
actually describe how to use them. Variables are either _universally
quantified_, corresponding to statements like "forall x, ..." or _existentially
quantified_, corresponding to statements like "there exists an x such that...".
In Allium, any variables declared in the head of an implication are universally
quantified, whereas any variables declared in the body of an implication are
existentially quantified.

It is possible to use existentially quantified variables to find a value which
satisfies a predicate. Allium has a feature called _backtracking_, where it will
enumerate possibilities until it finds a value with the property or runs out of
possibilities. For example, we can find a sundae that has chocolate in it. This
translates to first-order logic as "there exists a sundae `s` which contains
chocolate."

```
pred main {
    main <- containsChocolate(let s);
}
```

///////// ///////// ///////// ///////// ///////// ///////// ///////// ///////// 

## Complete Grammar

The following is a context-free grammar for Allium. Following the productions of this grammar, one should arrive at the same structure described by the output of invoking `allium` with `--print-syntactic-ast`.

Nonterminal symbols are enclosed in angle braces. ε denotes an empty rule, i.e. where the nonterminal may be rewritten as the empty string.

```
<Program> := <Type> <Program>
           | <Predicate> <Program>
           | ε

<Type> := type <TypeDecl> { <Constructor-list> }

<TypeDecl> := <identifier>

<Constructor-list> := <Constructor> <Constructor-list>
                    | ε

<Constructor> := ctor <identifier> ;
               | ctor <identifier> ( <TypeRef-list> ) ;

<TypeRef-list> := <TypeRef>
                | <TypeRef> , <TypeRef-list>

<TypeRef> := <identifier>

<Predicate> := pred <PredicateDecl> { <Implication-list> }

<PredicateDecl> := <identifier> ( <TypeRef-list> )

<Implication-list> := <Implication> <Implication-list>
                    | ε

<Implication> := <PredicateRef> <- <Expression> ;

<Expression> := <TruthValue>
              | <PredicateRef>
              | <Conjunction>

<TruthValue> := true
              | false

<PredicateRef> := <identifier>
                | <identifier> ( <Value-list> )

<Conjunction> := <Expression> , <Expression>

<Value-list> := <Value> <Value-list>
              | ε

<Value> := <identifer>
         | <identifier> ( <Value-list> )
         | let <identifier>

<identifier> := [a-zA-Z][a-zA-Z0-9]*
```
