#ifndef OPTIONAL_H
#define OPTIONAL_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>

/// Represents a value which may or may not have a value, and may be accessed
/// in a type-safe way.
///
/// Similar to std::optional, but with a monadic API.
template <typename T>
class Optional {
    static_assert(std::is_copy_constructible<T>::value, "T must be copy-constructible");
    static_assert(std::is_copy_assignable<T>::value, "T must be copy-assignable");
public:
    Optional(T t): wrapped(t) {}
    Optional(): wrapped() {}

    friend bool operator==(const Optional<T> &lhs, const Optional<T> &rhs) {
        return lhs.wrapped == rhs.wrapped;
    }

    friend inline bool operator!=(const Optional<T> &lhs, const Optional<T> &rhs) {
        return !(lhs == rhs);
    }

    /// An optional is truthy iff it has a value.
    ///
    /// Note that `Optional<int>(0)` converts to `true`.
    explicit operator bool () const { return wrapped.has_value(); }

    void map(std::function<void(T)> transform) const {
        if(wrapped.has_value()) {
            return transform(wrapped.value());
        }
    }

    /// Aplies a transformation to the value of this optional if it exists.
    template <typename U>
    Optional<U> map(std::function<U(T)> transform) const {
        if(wrapped.has_value()) {
            return Optional<U>(transform(wrapped.value()));
        } else {
            return Optional<U>();
        }
    }

    /// Applies a failable transformation to the value of this optional,
    /// collapsing optionals.
    template <typename U>
    Optional<U> flatMap(std::function< Optional<U> (T)> transform) const {
        if(wrapped.has_value()) {
            return transform(wrapped.value());
        } else {
            return Optional<U>();
        }
    }

    /// Calls the given observer with the value of this optional if there is one,
    /// and returns this optional again by reference for use in a chain.
    ///
    /// Example:
    /// ```
    /// Optional(5)
    ///     .then([](int x) { std::cout << x; })
    ///     .switchOver(...);
    /// ```
    const Optional &then(std::function<void(const T&)> observer) const {
        if(wrapped.has_value()) {
            observer(wrapped.value());
        }
        return *this;
    }

    /// Calls the given observer if there is no value in this observer, and
    /// returns this optional again by reference for use in a chain.
    ///
    /// Example:
    /// ```
    /// Optional<int>()
    ///     .error([]() { std::cout << "no value!"; })
    ///     .switchOver(...);
    /// ```
    const Optional &error(std::function<void(void)> observer) const {
        if(!wrapped.has_value()) {
            observer();
        }
        return *this;
    }

    template <typename U>
    U switchOver(std::function<U(T)> handleValue, std::function<U(void)> handleNone) const {
        if(wrapped.has_value()) {
            return handleValue(wrapped.value());
        } else {
            return handleNone();
        }
    }

    /// Gives the value of the optional if there is one, or falls back to the
    /// given value if there isn't one.
    T coalesce(T fallback) const {
        return wrapped.value_or(fallback);
    }

    /// Writes the value of the optional into the given variable if there is one
    /// and returns true on success.
    ///
    /// This is intended to unwrap optionals in a condition as can be done in
    /// languages which support pattern matching. The one downside is that the
    /// variable must already be defined, which is verbose at best and confusing
    /// if there is no default constructor for the wrapped type.
    ///
    /// Example:
    /// ```
    /// int x;
    /// if(Optional<int>(5).unwrapInto(x)) {
    ///     // now x = 5, and this wouldn't execute if the optional had no value!
    /// }
    /// ```
    bool unwrapInto(T &x) const {
        if(wrapped.has_value()) x = wrapped.value();
        return wrapped.has_value();
    }

    
    /// Writes the value of the optional into the given variable if there is one
    /// and returns false on success.
    ///
    /// This is intended to unwrap optionals similarly to `unwrapInto`, but requires
    /// that the function not continue execution if it returns false. In this way,
    /// one can avoid a level of nesting that would occur with unwrap into.
    ///
    /// Example:
    /// ```
    /// int x;
    /// if(Optional<int>(5).unwrapGuard(x)) {
    ///     // this code must exit the function!
    /// }
    /// // now x = 5, but would be uninitialized if the optional had no value!
    /// ```
    ///
    /// See also: `unwrapInto(T&)`
    bool unwrapGuard(T &x) const {
        if(wrapped.has_value()) x = wrapped.value();
        return !wrapped.has_value();
    }

    /// A version of `unwrapGuard` which borrows the value from the optional
    /// rather than making an owned copy.
    bool unwrapGuard(T *&x) {
        if(wrapped.has_value()) x = &wrapped.value();
        return !wrapped.has_value();
    }

    /// A version of `unwrapGuard` which makes an owned copy of the value from
    /// the optional.
    bool unwrapGuard(std::unique_ptr<T> &x) {
        if(wrapped.has_value()) x = std::make_unique<T>(wrapped.value());
        return !wrapped.has_value();
    }

    // TODO: this function should only be defined if T implements <<.
    friend std::ostream& operator<<(std::ostream& out, const Optional<T> o) {
        if(o.wrapped.has_value()) {
            return out << "some(" << o.wrapped.value() << ")";
        } else {
            return out << "none";
        }
    }
private:
    std::optional<T> wrapped;
};

#endif // OPTIONAL_H
