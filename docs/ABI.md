# Allium ABI

Allium does not guarantee ABI stability. This document aims to describe the ABI
anyway, which will hopefully be useful for Allium developers looking to
understand how programs are lowered to LLVM IR and eventually into assembly.

## Types

Consider an Allium type where none of the constructors have any arguments:
```
type ABC {
    ctor A;
    ctor B;
    ctor C;
}
```
The binary representation of `ABC` must have distinct representations for each
of the three constructors. There are two other possible "values" that are useful
to track at runtime: one for unbound variables of type `ABC` which can unify
with any value, and one for variables that have been unified with a value located
elsewhere in memory. These five cases are distinguished by separate "keys." The
two "special" runtime cases are common to all types, and therefore have the same
keys in all data types: 0 and 1 respectively. The constructors of the type have
tags which are ordered consecutively and start at 2. Thus, the tags for `ABC`
are as follows:

| Case    | Key |
|---------|-----|
| unbound | 0   |
| pointer | 1   |
| A       | 2   |
| B       | 3   |
| C       | 4   |

A "pointer" value, which has been unified with a value elsewhere in memory, also
needs to store the address of that other value. This address is called the
payload. For a type which doesn't have any constructors with arguments like
`ABC`, the payload of the type is the size of a pointer.

The complete layout of `ABC` includes the tag as an 8-bit number, followed by
the payload which has the size and alignment of a pointer. On x86-64, for
example, this puts 7 bytes of padding between the tag and the payload. The
layout should be the same as this C struct:
```
struct ABC {
    unsigned char tag;
    _Alignas(struct ABC *) union {
        struct ABC *pointer;
    } payload;
};
```

### Constructors with Arguments

The primary difference between the types discussed in the last section and those
with constructors that have arguments is in the payload. For any constructor
with arguments, its payload contains each of its arguments in order, with their
preferred alignment. The payload of the whole type has the size of its largest
constructor payload, and the alignment of its largest constructor payload
alignment.

For example, consider this Allium type:
```
type ABCPair {
    ctor ABCPair(ABC, ABC);
};
```

The layout should be the same as this C struct:
```
struct ABCPair {
    unsigned char tag;
    _Alignas(struct ABC) union {
        struct ABCPair *pointer;
        struct {
            ABC first;
            ABC second;
        } abcPair;
    } payload;
};
```

### Recursive Types

Some additional care is required for recursive and mutually recursive data
types. The representation of a type in memory should have a fixed size, so it is
essential that a type not be able to directly contain itself &mdash; thus,
arguments whose types are mutually recursive with the enclosing type are added
to the constructor's payload as pointers to that type instead of the type
itself. For a recursive type, this results in a single indirection; for mutually
recursive types it results in a pointer indirection in both types, rather than
arbitrarily choosing one.

For example, consider this natural number type:
```
type Nat {
    ctor Zero;
    ctor S(Nat);
}
```

This type should have a binary representation which is the same as this C
struct:
```
struct Nat {
    unsigned char tag;
    _Alignas(struct Nat *) union {
        struct Nat *pointer;
        struct {
            struct Nat *predecessor;
        } s;
    } payload;
};
```

And for an example of mutual recursion, consider these even/odd types:
```
type Even {
    ctor Zero;
    ctor S(Odd);
}

type Odd {
    ctor S(Even);
}
```

These should have binary representations equivalent to these C structs:
```
Struct Odd;

struct Even {
    unsigned char tag;
    _Alignas(struct Even *) union {
        struct Even *pointer;
        struct {
            // Note: indirection with pointer!
            struct Odd *predecessor;
        } s;
    } payload;
};

struct Odd {
    unsigned char tag;
    _Alignas(struct Odd *) union {
        struct Odd *pointer;
        struct {
            // Note: indirection with pointer!
            struct Even *predecessor;
        } s;
    } payload;
};
```
