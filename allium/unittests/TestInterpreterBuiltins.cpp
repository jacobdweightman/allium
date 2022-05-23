#include <gtest/gtest.h>

#include "Interpreter/BuiltinPredicates.h"
#include "Utils/Unit.h"

namespace interpreter {

class TestInterpreterBuiltinConcat : public testing::Test {
    void SetUp() {
        a = RuntimeValue(String("Hello"));
        b = RuntimeValue(String(" world!"));
        c = RuntimeValue(String("Hello world!"));

        ctx = { a, b, c };

        aVar = RuntimeValue(&ctx[0]);
        bVar = RuntimeValue(&ctx[1]);
        cVar = RuntimeValue(&ctx[2]);
    }

public:
    Context ctx;
    RuntimeValue a, b, c;
    RuntimeValue aVar, bVar, cVar;
};

TEST_F(TestInterpreterBuiltinConcat, fully_instantiated) {
    Generator<Unit> g = concat({ a, b, c });
    EXPECT_TRUE(g.next());
    EXPECT_FALSE(g.next());
}

TEST_F(TestInterpreterBuiltinConcat, concat_fully_instantiated_with_indirection) {
    Generator<Unit> g = concat({ aVar, bVar, cVar });
    EXPECT_TRUE(g.next());
    EXPECT_FALSE(g.next());
}

TEST_F(TestInterpreterBuiltinConcat, concat_c_uninstantiated) {
    ctx[2] = RuntimeValue();
    Generator<Unit> g = concat({ a, b, cVar });
    EXPECT_TRUE(g.next());
    EXPECT_EQ(ctx[2], RuntimeValue(String("Hello world!")));
    EXPECT_FALSE(g.next());
}

} // namespace interpreter
