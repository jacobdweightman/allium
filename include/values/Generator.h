#include <exception>
#include <iostream>

#if defined(__clang__)
    #include <experimental/coroutine>

    using std::experimental::suspend_always;
    using std::experimental::suspend_never;
    using std::experimental::coroutine_handle;
#else
    #include <coroutine>

    using std::suspend_always;
    using std::suspend_never;
    using std::coroutine_handle;
#endif

#include "values/Optional.h"

template <typename T>
struct Generator {
    struct promise_type {
        bool done = false;
        T value;

        suspend_always initial_suspend() { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() {}

        Generator<T> get_return_object() {
            return Generator{ coroutine_handle<promise_type>::from_promise(*this) };
        }

        suspend_always yield_value(T x) noexcept {
            value = x;
            return {};
        }

        suspend_always final_suspend() noexcept {
            done = true;
            return {};
        }
    };

    Generator() {}
    Generator(coroutine_handle<promise_type> h): handle(h) {}

    ~Generator() {
        handle.destroy();
    }

    Optional<T> next() {
        handle.resume();
        auto &promise = handle.promise();
        if(!promise.done) {
            return promise.value;
        } else {
            return Optional<T>();
        }
    }

private:
    coroutine_handle<promise_type> handle;
};
