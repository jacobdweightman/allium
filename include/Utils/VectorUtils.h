#include <functional>
#include <vector>

#include "Utils/Optional.h"

/// Produces a vector whose entries are the result of applying the given
/// transformation to each element of the given vector.
template <typename T, typename U>
std::vector<U> map(std::vector<T> V, std::function<U(T)> f) {
    std::vector<U> result;
    result.reserve(V.size());
    for(T v : V) {
        result.push_back(f(v));
    }
    return result;
}

/// Applies the given tranformation to each element of the given vector,
/// returning a vector of the results where the transformation produced a value.
template <typename T, typename U>
std::vector<U> compactMap(std::vector<T> V, std::function< Optional<U> (T)> f) {
    std::vector<U> result;
    result.reserve(V.size());
    for(T v : V) {
        f(v).map([&](U u) { result.push_back(u); });
    }
    return result;
}

/// Applies the given transformation to each element of the given vector,
/// returning a vector of the results if all of them succeed or an optional with
/// no value if any of them fail.
template <typename T, typename U>
Optional<std::vector<U>> fullMap(std::vector<T> V, std::function< Optional<U> (T)> f) {
    std::vector<U> result;
    result.reserve(V.size());
    for(T v : V) {
        Optional<U> u = f(v);
        if(!u) return Optional<std::vector<U> >();
        u.map([&](U u) { result.push_back(u); });
    }
    return result;
}
