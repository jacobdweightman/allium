#ifndef TAGGED_UNION_H
#define TAGGED_UNION_H

#include <algorithm>
#include <assert.h>
#include <functional>
#include <iostream>
#include <tuple>
#include <variant>

#include "Optional.h"

/// Exposes the type of a handler for a given associated value type.
template <typename T>
struct handler {
    typedef std::function<void(T&)> type;
};

template <typename T, typename U>
struct matcher {
    typedef std::function<U(T&)> type;
};

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename ... Ts>
class TaggedUnion {
protected:
    std::variant<Ts...> wrapped;

public:
    TaggedUnion(typename std::tuple_element<0, std::tuple<Ts...>>::type t): wrapped(t) {}
    explicit TaggedUnion(std::variant<Ts...> value): wrapped(value) {}

    friend bool operator==(const TaggedUnion &lhs, const TaggedUnion &rhs) {
        return lhs.wrapped == rhs.wrapped;
    }

    friend inline bool operator!=(const TaggedUnion &lhs, const TaggedUnion &rhs) {
        return !(lhs == rhs);
    }

    template <typename T>
    T match(typename matcher<Ts, T>::type... matchers) const {
        // Without storing wrapped into a temporary, std::visit complains that
        // the matchers are non-exhaustive (at least in Clang).
        auto wr = wrapped;
        return std::visit(overloaded { matchers... }, wr);
    }

    void switchOver(typename handler<Ts>::type... handlers) const {
        // Without storing wrapped into a temporary, std::visit complains that
        // the matchers are non-exhaustive (at least in Clang).
        auto wr = wrapped;
        return std::visit(overloaded { handlers... }, wr);
    }

    template <typename T>
    Optional<T> as_a() const {
        if(auto *x = std::get_if<T>(&wrapped)) {
            return *x;
        } else {
            return Optional<T>();
        }
    }
};

// TODO: the constructor does not work as expected when case types are
// implicitly convertable.

#endif // TAGGED_UNION_H
