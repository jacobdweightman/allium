#ifndef OPTIONAL_H
#define OPTIONAL_H

#include <algorithm>
#include <functional>
#include <iostream>

/// Represents a value which may or may not have a value, and may be accessed
/// in a type-safe way.
///
/// Similar to std::optional, but with a monadic API.
template <typename T>
class Optional {
    static_assert(std::is_copy_constructible<T>::value, "T must be copy-constructible");
    static_assert(std::is_copy_assignable<T>::value, "T must be copy-assignable");
public:
    Optional(T t): has_value(true), value(t) {}
    Optional(): has_value(false), value() {}

    Optional(const Optional &other): has_value(other.has_value) {
        if(has_value) {
            new (&value.wrapped) auto(other.value.wrapped);
        }
    }

    ~Optional() {
        if(has_value) value.wrapped.~T();
    }

    Optional<T> operator=(Optional<T> other) {
        swap(*this, other);
        return *this;
    }

    friend bool operator==(const Optional<T> &lhs, const Optional<T> &rhs) {
        return lhs.has_value == rhs.has_value &&
            (!lhs.has_value || lhs.value.wrapped == rhs.value.wrapped);
    }

    friend inline bool operator!=(const Optional<T> &lhs, const Optional<T> &rhs) {
        return !(lhs == rhs);
    }

    /// An optional is truthy iff it has a value.
    explicit operator bool () const { return has_value; }

    friend void swap(Optional &lhs, Optional &rhs) {
        if(lhs.has_value) {
            auto tmp = lhs.value.wrapped;
            lhs.value.wrapped.~T();
            if(rhs.has_value) {
                new (&lhs.value.wrapped) auto(rhs.value.wrapped);
                rhs.value.wrapped.~T();
            }
            new (&rhs.value.wrapped) auto(tmp);
        } else {
            if(rhs.has_value) {
                new (&lhs.value.wrapped) auto(rhs.value.wrapped);
                rhs.value.wrapped.~T();
            }
        }
        std::swap(lhs.has_value, rhs.has_value);
    }

    /// Aplies a transformation to the value of this optional if it exists.
    template <typename U>
    Optional<U> map(std::function<U(T)> transform) const {
        if(has_value) {
            return Optional<U>(transform(value.wrapped));
        } else {
            return Optional<U>();
        }
    }

    /// Applies a failable transformation to the value of this optional,
    /// collapsing optionals.
    template <typename U>
    Optional<U> flatMap(std::function< Optional<U> (T)> transform) const {
        if(has_value) {
            return transform(value.wrapped);
        } else {
            return Optional<U>();
        }
    }

    template <typename U>
    U switchOver(std::function<U(T)> handleValue, std::function<U(void)> handleNone) const {
        if(has_value) {
            return handleValue(value.wrapped);
        } else {
            return handleNone();
        }
    }

    /// Gives the value of the optional if there is one, or falls back to the
    /// given value if there isn't one.
    T coalesce(T fallback) const {
        if(has_value) {
            return value.wrapped;
        } else {
            return fallback;
        }
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
        if(has_value) x = value.wrapped;
        return has_value;
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
        if(has_value) x = value.wrapped;
        return !has_value;
    }

    // TODO: this function should only be defined if T implements <<.
    friend std::ostream& operator<<(std::ostream& out, const Optional<T> o) {
        if(o.has_value) {
            return out << "some(" << o.value.wrapped << ")";
        } else {
            return out << "none";
        }
    }
private:
    /// This wrapper allows default construction of an optional without a
    /// default constructor for the type of the value.
    union Wrapper {
        Wrapper(T t): wrapped(t) {}
        Wrapper() {}
        
        // Give an explicit do-nothing destructor. Destruction of the wrapped
        // value, if it exists, is handled in the Optional destructor.
        ~Wrapper() {}

        T wrapped;
    };

    bool has_value;
    Wrapper value;
};

#endif // OPTIONAL_H
