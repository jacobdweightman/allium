#include <gtest/gtest.h>

#include "Utils/Optional.h"

template class Optional<int>;


TEST(TestOptional, constructors) {
    // Just verify that these constructions are allowed.
    Optional<int> a(5);
    Optional<int> b;
    Optional<int> c = a;

    EXPECT_NE(&c, &a);
}

TEST(TestOptional, copy_assignment) {
    // Note: this test depends on unwrapInto in order to make assertions on the
    // contents of the optionals.
    Optional<std::string> a("hello");
    Optional<std::string> b("world");
    
    a = b;
    EXPECT_EQ(a, b);
    std::string s;
    if(a.unwrapInto(s)) {
        EXPECT_STREQ(s.c_str(), "world");
    } else {
        FAIL();
    }
    std::string t;
    if(b.unwrapInto(t)) {
        EXPECT_STREQ(s.c_str(), "world");
    } else {
        FAIL();
    }
}

class ComplexObjectMock {
public:
    ComplexObjectMock() { ++constructed_count; }
    ComplexObjectMock(const ComplexObjectMock &other) {
        ++constructed_count;
    }
    ~ComplexObjectMock() { ++destructed_count; }

    static int constructed_count;
    static int destructed_count;
};
int ComplexObjectMock::constructed_count = 0;
int ComplexObjectMock::destructed_count = 0;

TEST(TestOptional, calls_destructor_when_deallocated) {
    if(true) {
        // x is deallocated at the end of the condition.
        auto x = Optional(ComplexObjectMock());
    }

    EXPECT_EQ(
        ComplexObjectMock::constructed_count,
        ComplexObjectMock::destructed_count
    );
}

TEST(TestOptional, equatable) {
    Optional<int> a(5);
    Optional<int> b(5);
    Optional<int> c(6);
    Optional<int> d;
    Optional<int> e;

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a == d);
    EXPECT_TRUE(a != d);
    EXPECT_TRUE(d == e);
    EXPECT_FALSE(d != e);
}

TEST(TestOptional, toBool) {
    EXPECT_TRUE(Optional<int>(5));
    EXPECT_TRUE(Optional<int>(0));
    EXPECT_FALSE(Optional<int>());
}

TEST(TestOptional, map) {
    // A value to capture in closures, which previously exposed a bug.
    int diff = 4;

    // The closure must run.
    EXPECT_EQ(
        Optional<int>(5).map<int>([diff](int x) -> int { return x + diff; }),
        Optional<int>(9)
    );

    // The closure must not run.
    Optional<int>().map<int>([&diff](int x) -> int {
        EXPECT_TRUE(false);
        return x + diff;
    });
}

TEST(TestOptional, flatMap) {
    // A value to capture in closures, which previously exposed a bug.
    int diff = 3;

    // The closure must run.
    EXPECT_EQ(
        Optional<int>(5).flatMap<int>([diff](int x) -> Optional<int> {
            return Optional<int>(x + diff);
        }),
        Optional<int>(8)
    );

    // The closure must not run.
    EXPECT_EQ(
        Optional<int>().flatMap<int>([&diff](int x) -> Optional<int> {
            EXPECT_TRUE(false);
            return Optional<int>(x + diff);
        }),
        Optional<int>()
    );
}

TEST(TestOptional, switchOver) {
    EXPECT_EQ(
        Optional<int>(7).switchOver<int>(
            [](int x) -> int { return x + 3; },
            []() -> int { EXPECT_TRUE(false); return 0; }
        ),
        10
    );

    EXPECT_EQ(
        Optional<int>().switchOver<int>(
            [](int x) -> int { EXPECT_TRUE(false); return x + 3; },
            []() -> int { return 0; }
        ),
        0
    );
}

TEST(TestOptional, coalesce) {
    EXPECT_EQ(Optional<int>(5).coalesce(0), 5);
    EXPECT_EQ(Optional<int>().coalesce(0), 0);
}

TEST(TestOptional, unwrapInto) {
    bool was_called = false;
    int x;
    EXPECT_TRUE(Optional<int>(5).unwrapInto(x));
    EXPECT_EQ(x, 5);

    EXPECT_FALSE(Optional<int>().unwrapInto(x));
}

TEST(TestOptional, unwrapGuard) {
    int x;
    EXPECT_FALSE(Optional<int>(5).unwrapGuard(x));
    EXPECT_EQ(x, 5);

    EXPECT_TRUE(Optional<int>().unwrapGuard(x));
}

TEST(TestOptional, all_with_all_values) {
    auto result = all<int,int,int>(
        Optional<int>(2),
        Optional<int>(5),
        Optional<int>(14));

    EXPECT_EQ(
        result,
        std::make_tuple(2, 5, 14)
    );
}

TEST(TestOptional, all_with_missing_values) {
    using Result = Optional<std::tuple<int,int,int>>;

    // Since EXPECT_EQ is a macro, it doesn't like templates with multiple
    // template parameters to occur inside of it.
    Result x = all<int,int,int>(Optional<int>(), Optional<int>(5), Optional<int>(14));
    EXPECT_EQ(x, Result());
    x = all<int,int,int>(Optional<int>(2), Optional<int>(), Optional<int>(14));
    EXPECT_EQ(x, Result());
    x = all<int,int,int>(Optional<int>(2), Optional<int>(5), Optional<int>());
    EXPECT_EQ(x, Result());
    x = all<int,int,int>(Optional<int>(), Optional<int>(), Optional<int>(14));
    EXPECT_EQ(x, Result());
    x = all<int,int,int>(Optional<int>(2), Optional<int>(), Optional<int>());
    EXPECT_EQ(x, Result());
    x = all<int,int,int>(Optional<int>(), Optional<int>(5), Optional<int>());
    EXPECT_EQ(x, Result());
    x = all<int,int,int>(Optional<int>(), Optional<int>(), Optional<int>());
    EXPECT_EQ(x, Result());
}
