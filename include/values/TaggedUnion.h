#ifndef TAGGED_UNION_H
#define TAGGED_UNION_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <tuple>
#include <assert.h>

#include "Optional.h"

/// Exposes the type of a handler for a given associated value type.
template <typename T>
struct handler {
    typedef std::function<void(T)> type;
};

template <typename T, typename U>
struct matcher {
    typedef std::function<U(T)> type;
};

/// A utility for representing and accessing sum types in a type-safe way.
///
/// Restriction: all associated value types must be distinct.
/// Restriction: destructor of value is never called. TODO: address this
template <typename Head, typename ... Tail>
class TaggedUnion {
    static_assert(std::is_copy_constructible<Head>::value,
        "TaggedUnion type parameters must be copy-constructible");
    static_assert(std::is_copy_constructible<TaggedUnion<Tail...> >::value,
        "TaggedUnion tail parameters must be copy-constructible");
public:
    TaggedUnion(Head h): is_active_case(true), value(h) {}

    template <typename T>
    TaggedUnion(T t): is_active_case(false), value(TaggedUnion<Tail...>(t)) {
        static_assert(!std::is_same<T, Head>::value,
            "This constructor shouldn't be used with type 'Head'");
    }

    TaggedUnion(const TaggedUnion &other): is_active_case(other.is_active_case) {
        if(is_active_case) {
            new (&value.head) auto(other.value.head);
        } else {
            new (&value.tail) auto(other.value.tail);
        }
    }

    ~TaggedUnion() {
        if(is_active_case) {
            value.head.~Head();
        } else {
            value.tail.~TaggedUnion<Tail...>();
        }
    }

    TaggedUnion<Head, Tail...> operator=(TaggedUnion<Head, Tail...> other) {
        swap(*this, other);
        return *this;
    }

    bool operator=(const Optional<TaggedUnion> &other) {
        other.switchOver(
            [this, &other](TaggedUnion u) { *this = other; },
            []() {}
        );
        return bool(other);
    }

    friend bool operator==(const TaggedUnion &lhs, const TaggedUnion &rhs) {
        if(lhs.is_active_case || rhs.is_active_case) {
            return lhs.is_active_case && rhs.is_active_case &&
                lhs.value.head == rhs.value.head;
        } else {
            return lhs.value.tail == rhs.value.tail;
        }
    }

    friend bool operator!=(const TaggedUnion &lhs, const TaggedUnion &rhs) {
        return !(lhs == rhs);
    }

    friend void swap(TaggedUnion &lhs, TaggedUnion &rhs) {
        using std::swap;
        // TODO: refactor this logic to swap value unions.
        if(lhs.is_active_case) {
            auto tmp = lhs.value.head;
            lhs.value.head.~Head();
            if(rhs.is_active_case) {
                new (&lhs.value.head) auto(rhs.value.head);
                rhs.value.head.~Head();
            } else {
                new (&lhs.value.tail) auto(rhs.value.tail);
                rhs.value.tail.~TaggedUnion<Tail...>();
            }
            new (&rhs.value.head) auto(tmp);
        } else {
            auto tmp = lhs.value.tail;
            lhs.value.tail.~TaggedUnion<Tail...>();
            if(rhs.is_active_case) {
                new (&lhs.value.head) auto(rhs.value.head);
                rhs.value.head.~Head();
            } else {
                new (&lhs.value.tail) auto(rhs.value.tail);
                rhs.value.tail.~TaggedUnion<Tail...>();
            }
            new (&rhs.value.tail) auto(tmp);
        }
        swap(lhs.is_active_case, rhs.is_active_case);
    }

    template <typename T>
    T match(
        typename matcher<Head, T>::type headMatcher,
        typename matcher<Tail, T>::type... tailMatchers
    ) const {
        if(is_active_case) {
            return headMatcher(value.head);
        } else {
            return value.tail.template match<T>(tailMatchers...);
        }
    }

    void switchOver(
        typename handler<Head>::type headHandler,
        typename handler<Tail>::type... tailHandlers
    ) const {
        if(is_active_case) {
            return headHandler(value.head);
        } else {
            return value.tail.switchOver(tailHandlers...);
        }
    }

    template <typename T>
    Optional<T> as_a() const {
        if constexpr (std::is_same<T, Head>::value) {
            if(is_active_case) {
                return Optional<T>(static_cast<T>(value.head));
            } else {
                return Optional<T>();
            }
        } else {
            if(is_active_case) {
                return Optional<T>();
            } else {
                return value.tail.template as_a<T>();
            }
        }
    }

private:
    union Value {
        Value() {}
        Value(Head h): head(h) {}
        Value(TaggedUnion<Tail...> t): tail(t) {}

        // Give an explicit do-nothing destructor. Destruction of head, if it
        // is active, is handled in the TaggedUnion destructor.
        ~Value() {}

        Head head;
        TaggedUnion<Tail...> tail;
    };

    bool is_active_case;
    Value value;
};

template <typename T> 
class TaggedUnion<T> {
    static_assert(std::is_copy_constructible<T>::value,
        "TaggedUnion type parameters must be copy-constructible");
    static_assert(std::is_copy_assignable<T>::value,
        "TaggedUnion type parameters must be copy-assignable");
public:
    TaggedUnion(T t): value(t) {}
    TaggedUnion(const TaggedUnion &other): value(other.value) {}
    ~TaggedUnion() = default;

    TaggedUnion<T> operator=(TaggedUnion other) {
        using std::swap;
        swap(value, other.value);
        return *this;
    }

    friend bool operator==(const TaggedUnion &lhs, const TaggedUnion &rhs) {
        return lhs.value == rhs.value;
    }

    friend inline bool operator!=(const TaggedUnion &lhs, const TaggedUnion &rhs) {
        return !(lhs == rhs);
    }

    template <typename U>
    U match(typename matcher<T, U>::type headMatcher) const {
        return headMatcher(value);
    }

    void switchOver(typename handler<T>::type handler) const {
        return handler(value);
    }

    template <typename U>
    Optional<U> as_a() const {
        if constexpr (std::is_same<T, U>::value) {
            return Optional<T>(value);
        } else {
            return Optional<U>();
        }
    }

private:
    T value;
};

// TODO: the constructor does not work as expected when case types are
// implicitly convertable.

#endif // TAGGED_UNION_H
