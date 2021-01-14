#include "gtest/gtest.h"
#include "values/TaggedUnion.h"

struct A {
    A() { ++constructed_count; }
    A(const A&) { ++constructed_count; }
    ~A() { ++destructed_count; }

    friend bool operator==(const A &left, const A &right) {
        return true;
    }
    friend bool operator!=(const A &left, const A &right) {
        return false;
    }
    friend std::ostream& operator<<(std::ostream &out, const A &a) {
        return out << "A()";
    }
    static int constructed_count;
    static int destructed_count;
};
int A::constructed_count;
int A::destructed_count;

struct B {
    friend bool operator==(const B &left, const B &right) {
        return true;
    }
    friend bool operator!=(const B &left, const B &right) {
        return false;
    }
    friend std::ostream& operator<<(std::ostream &out, const B &a) {
        return out << "B()";
    }
};
struct C {
    friend bool operator==(const C &left, const C &right) {
        return true;
    }
    friend bool operator!=(const C &left, const C &right) {
        return false;
    }
    friend std::ostream& operator<<(std::ostream &out, const C &a) {
        return out << "C()";
    }
};

class TestTaggedUnion : public testing::Test {
protected:
    void SetUp() override {}

    TaggedUnion<A, B, C>
        au = TaggedUnion<A, B, C>(A()),
        bu = TaggedUnion<A, B, C>(B()),
        cu = TaggedUnion<A, B, C>(C());
};

TEST_F(TestTaggedUnion, match) {
    EXPECT_TRUE(au.match<bool>(
        [](A) { return true; },
        [](B) { return false; },
        [](C) { return false; }
    ));
}

TEST_F(TestTaggedUnion, equatable) {
    EXPECT_EQ(au, au);
    EXPECT_NE(au, bu);
    EXPECT_NE(au, cu);

    EXPECT_NE(bu, au);
    EXPECT_EQ(bu, bu);
    EXPECT_NE(bu, cu);

    EXPECT_NE(cu, au);
    EXPECT_NE(cu, bu);
    EXPECT_EQ(cu, cu);
}

TEST_F(TestTaggedUnion, calls_destructor_when_deallocated) {
    A::constructed_count = 0;
    A::destructed_count = 0;

    if(true) {
        auto a = TaggedUnion<A>(A());
    }

    EXPECT_EQ(A::constructed_count, A::destructed_count);
}

TEST_F(TestTaggedUnion, switchOver) {
    bool was_called = false;
    au.switchOver(
        [&](A a) { was_called = true; },
        [](B b) { FAIL(); },
        [](C c) { FAIL(); }
    );
    EXPECT_TRUE(was_called);

    was_called = false;
    bu.switchOver(
        [](A a) { FAIL(); },
        [&](B b) { was_called = true; },
        [](C c) { FAIL(); }
    );
    EXPECT_TRUE(was_called);

    was_called = false;
    cu.switchOver(
        [](A a) { FAIL(); },
        [](B b) { FAIL(); },
        [&](C c) { was_called = true; }
    );
    EXPECT_TRUE(was_called);
}

TEST_F(TestTaggedUnion, as_a) {
    EXPECT_EQ(au.as_a<A>(), A());
    EXPECT_EQ(au.as_a<B>(), Optional<B>());
    EXPECT_EQ(au.as_a<C>(), Optional<C>());

    EXPECT_EQ(bu.as_a<A>(), Optional<A>());
    EXPECT_EQ(bu.as_a<B>(), B());
    EXPECT_EQ(bu.as_a<C>(), Optional<C>());

    EXPECT_EQ(cu.as_a<A>(), Optional<A>());
    EXPECT_EQ(cu.as_a<B>(), Optional<B>());
    EXPECT_EQ(cu.as_a<C>(), C());
}
