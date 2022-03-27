# Allium — Parametric Polymorphism

I often find myself needing a `List` type. Due to Allium’s strong static
typing, I usually have to specialize this code for different element types:
```
type StringList {
    ctor Nil;
    ctor Cons(String, StringList);
}

pred inStringList(String, StringList) {
    inStringList(let x, Cons(x, _)) <- true;
    inStringList(let x, cons(_, let t)) <- in(x, t);
}

type IntList {
    ctor Nil;
    ctor Cons(Int, IntList);
}

pred inIntList(String, StringList) {
    inIntList(let x, Cons(x, _)) <- true;
    inIntList(let x, cons(_, let t)) <- in(x, t);
}

...
```

######### ######### ######### ######### ######### ######### ######### ######### 
It would be great to be able to maintain strong static typing while also
deduplicating this code. Different languages take a number of different
approaches, like inheritance/subtyping or traits, but I think parametric
polymorphism is the best fit for Allium right now — we could consider other
extensions in the future, though. What I’d like to write is this:
```
type List<T> {
    ctor Nil;
    ctor Cons(T, List<T>);
}

pred in<T>(T, List<T>) {
    in(let x, Cons(x, _)) <- true;
    in(let x, Cons(_, let t)) <- in(x, t);
}
```

This would probably go into the standard library that we’ll have someday. This
is, of course, useful for more than just lists — here are some other
polymorphic things I’d like to have in the standard library:
```
pred asString<T>(T, String)
pred equal<T>(T, T)
pred less<T>(T, T)
pred greater<T>(T, T)
type Tree<T>
type TreeSet<T>
```

