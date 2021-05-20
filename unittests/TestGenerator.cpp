#include <gtest/gtest.h>

#include "values/Generator.h"

Generator<int> finite() {
    co_yield 1;
    co_yield 2;
}

TEST(TestGenerator, finite_generator_returns_none_when_exhausted) {
    auto f = finite();
    EXPECT_EQ(f.next(), 1);
    EXPECT_EQ(f.next(), 2);
    EXPECT_EQ(f.next(), Optional<int>());
}

Generator<int> counter() {
    for(int i=0;; ++i) {
        co_yield i;
    }
}

TEST(TestGenerator, counter) {
    auto c = counter();
    EXPECT_EQ(c.next(), 0);
    EXPECT_EQ(c.next(), 1);
    EXPECT_EQ(c.next(), 2);
    EXPECT_EQ(c.next(), 3);
    EXPECT_EQ(c.next(), 4);
    EXPECT_EQ(c.next(), 5);
    EXPECT_EQ(c.next(), 6);
}

Generator<int> fibonacci() {
    int a = 1;
    int b = 1;

    while(true) {
        co_yield a;
        int tmp = a + b;
        a = b;
        b = tmp;
    }
}

TEST(TestGenerator, fibonacci) {
    auto fib = fibonacci();
    EXPECT_EQ(fib.next(), 1);
    EXPECT_EQ(fib.next(), 1);
    EXPECT_EQ(fib.next(), 2);
    EXPECT_EQ(fib.next(), 3);
    EXPECT_EQ(fib.next(), 5);
    EXPECT_EQ(fib.next(), 8);
    EXPECT_EQ(fib.next(), 13);
}

Generator<int> nested() {
    auto c = counter();
    auto f = fibonacci();

    while(true) {
        int cn, fn;
        if(c.next().unwrapGuard(cn))
            break;
        if(f.next().unwrapGuard(fn))
            break;
        co_yield cn + fn;
    }
}

TEST(TestGenerator, nested) {
    auto n = nested();
    EXPECT_EQ(n.next(), 1);
    EXPECT_EQ(n.next(), 2);
    EXPECT_EQ(n.next(), 4);
    EXPECT_EQ(n.next(), 6);
    EXPECT_EQ(n.next(), 9);
    EXPECT_EQ(n.next(), 13);
    EXPECT_EQ(n.next(), 19);
}

Generator<int> powers(int n) {
    if(n == 0) {
        while(true)
            co_yield 1;
    }

    auto pred = powers(n-1);
    int m;
    for(int i=0; pred.next().unwrapInto(m); ++i)
        co_yield i * m;
}

TEST(TestGenerator, recursive) {
    auto r = powers(3);
    EXPECT_EQ(r.next(), 0);
    EXPECT_EQ(r.next(), 1);
    EXPECT_EQ(r.next(), 8);
    EXPECT_EQ(r.next(), 27);
    EXPECT_EQ(r.next(), 64);
    EXPECT_EQ(r.next(), 125);
    EXPECT_EQ(r.next(), 216);
}
