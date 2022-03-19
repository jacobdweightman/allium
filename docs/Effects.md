# Effects and Handlers in Allium

The functionality described in this document is a work in progress. It is not
yet available, but is included to document the design in an accessible way and
as a starting point for contributors.

In an effort to have a working "hello world" application in Allium, I added
minimal support for effects and a builtin `IO` effect type (commit 21a683c5797).
For Allium's effect system to be minimally viable, we need to make it possible
to handle effects within Allium code.

To the programmer, effects are the bridge between "logic" and "everything else."
Computer programs do all kinds of things, like printing text, manipulating
files, and displaying user interfaces — but these things don't correspond
closely with mathematical proofs or "abstract computation" (the thing a Turing
machine does). Someday, Allium will provide an interface for effectful
libraries, which can define effect types and default handlers and make it
possible to do all of these things in Allium programs.

Today, the bridge between logic and effects is one-way. It is possible to
perform an effect from a predicate:
```
pred main: IO {
    main <- do print("Hello world!");
}
```

But there is no way to "handle" that effect, thus defining the logical "meaning"
of the effect. This works for `IO.print(in String)` because it has a default
builtin handler defined in the interpreter which performs the effect and is
always true. An effect handler in an Allium program should "take care of" an
effect by performing 0 or more other effects and providing a precise logical
meaning to the effect call site.

Consider a program that needs to print logs during its execution. Printing a log
to the console is effectful, so we model it with an effect type:
```
type LogLevel {
    ctor Info;
    ctor Warning;
    ctor Error;
}

effect Log {
    ctor message(in LogLevel, in String);
}
```

The program can then print the logs to the console by "lowering" them to
`IO.print` effects:
```
pred p: IO {
    main <- do message(Info, "The program started running!"),
        do message(Warning, "Something looks fishy here..."),
        do message(Error, "... and now we have to exit with failure.");

    handle Log {
        message(Info, let s) <- do print("Info:"), do print(s);
        message(Warning, let s) <- do print("Warning:"), do print(s);
        message(Error, let s) <- do print("Error:"), do print(s),
            false;
    }
}

pred main: IO {
    main <- p;
}
```

This handler gives the following meaning to the `Log.message` effect:
* `Info` messages are printed with the prefix "Info: ", and are logically
  equivalent to `true`.
* `Warning` messages are printed with the prefix "Warning: ", and are logically
  equivalent to `true`.
* `Error` messages are printed with the prefix "Error: ", and are logically
  equivalent to false`, so that the current proof backtracks from the error.

It's possible to give other meanings to the same "effects." For example, one
could silence all log messages like this:
```
pred q: Log { ... }

pred main {
    main <- q;

    handle Log {
        message(_, _) <- true;
    }
}
```

Note that the signature of `p` is only annotated with the `IO` effect type: even
though `Log` effects may happen inside a proof of `main`, they are "handled" by
the handler and so no predicate which uses `p` in a subproof (like `main`) will
ever see `Log`. This gives effects a nice compositional property: effects
handled by a predicate are totally transparent to the caller, so the caller only
needs to consider a predicate's unhandled effects (which are part of its
signature).
