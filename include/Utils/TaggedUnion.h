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
    typedef std::function<void(T)> type;
};

template <typename T, typename U>
struct matcher {
    typedef std::function<U(T)> type;
};

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

    template <typename T, size_t N = 0>
    T match(typename matcher<Ts, T>::type... matchers) const {
        auto matchers_tuple = std::make_tuple(matchers...);
        if(N == wrapped.index()) {
            auto matcher = std::get<N>(matchers_tuple);
            return matcher(std::get<N>(wrapped));
        }
        if constexpr (N + 1 < std::tuple_size_v<decltype(matchers_tuple)>) {
            return match<T, N+1>(matchers...);
        }
        throw "impossible!";
    }

    template <size_t N = 0>
    void switchOver(typename handler<Ts>::type... handlers) const {
        auto handlers_tuple = std::make_tuple(handlers...);
        if(N == wrapped.index()) {
            auto handler = std::get<N>(handlers_tuple);
            handler(std::get<N>(wrapped));
        } else if constexpr (N + 1 < std::tuple_size_v<decltype(handlers_tuple)>) {
            switchOver<N+1>(handlers...);
        }
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
