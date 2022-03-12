#include <gtest/gtest.h>

#include "JSON.h"
#include "LSPTypes.h"

TEST(TestLanguageServerDecoding, decode_unit) {
    JSON json;
    Optional<Unit> y;

    json = JSON(Unit());
    y = decodeJSON<Unit>(json);
    EXPECT_EQ(y, Unit());

    json = JSON(JSONArray());
    y = decodeJSON<Unit>(json);
    EXPECT_EQ(y, Optional<Unit>());
}

TEST(TestLanguageServerDecoding, decode_bool) {
    JSON json;
    Optional<bool> b;

    json = JSON(true);
    b = decodeJSON<bool>(json);
    EXPECT_EQ(b, true);

    json = JSON(false);
    b = decodeJSON<bool>(json);
    EXPECT_EQ(b, false);

    json = JSON(JSONArray());
    b = decodeJSON<bool>(json);
    EXPECT_EQ(b, Optional<bool>());

    json = JSON(Unit());
    b = decodeJSON<bool>(json);
    EXPECT_EQ(b, Optional<bool>());
}

TEST(TestLanguageServerDecoding, decode_double) {
    JSON json;
    Optional<double> d;

    json = JSON(3.14159);
    d = decodeJSON<double>(json);
    EXPECT_EQ(d, 3.14159);

    json = JSON((double) 42);
    d = decodeJSON<double>(json);
    EXPECT_EQ(d, 42);

    json = JSON(JSONArray());
    d = decodeJSON<double>(json);
    EXPECT_EQ(d, Optional<double>());
}

TEST(TestLanguageServerDecoding, decode_string) {
    JSON json;
    Optional<std::string> str;

    json = JSON("");
    str = decodeJSON<std::string>(json);
    EXPECT_EQ(str, std::string(""));

    json = JSON("Hello world!");
    str = decodeJSON<std::string>(json);
    EXPECT_EQ(str, std::string("Hello world!"));

    json = JSON(JSONArray());
    str = decodeJSON<std::string>(json);
    EXPECT_EQ(str, Optional<std::string>());
}

TEST(TestLanguageServerDecoding, decode_vector) {
    JSON json;
    Optional<std::vector<double>> arr;

    json = JSON(JSONArray({ JSON(1.0), JSON(2.0), JSON(3.0) }));
    arr = decodeJSONArray<double>(json);
    std::vector<double> result = { 1.0, 2.0, 3.0 };
    EXPECT_EQ(arr, result);
}

TEST(TestLanguageServerDecoding, decode_tagged_union) {
    JSON json;
    Optional<TaggedUnion<bool, double>> tu;

    json = JSON(true);
    tu = decodeJSONUnion<bool, double>(json);
    auto result = Optional<TaggedUnion<bool, double>>(true);
    EXPECT_EQ(tu, result);

    json = JSON(225.0);
    tu = decodeJSONUnion<bool, double>(json);
    result = TaggedUnion<bool, double>((double) 225.0);
    EXPECT_EQ(tu, result);

    json = JSON(Unit());
    tu = decodeJSONUnion<bool, double>(json);
    result = Optional<TaggedUnion<bool, double>>();
    EXPECT_EQ(tu, result);
}

TEST(TestLanguageServerDecoding, decode_InitializeParams) {
    JSON json = JSON(JSONObject({
        { "processId", JSON(Unit()) },
        { "clientInfo", JSON(JSONObject({
            { "name", JSON("Visual Studio Code") },
            { "version", JSON("1.59.1") }
        })) }
    }));
    EXPECT_EQ(
        InitializeParams::decode(json),
        InitializeParams(
            TaggedUnion<int, Unit>(Unit()),
            ClientInfo(
                std::string("Visual Studio Code"),
                std::string("1.59.1")
            )
        )
    );
}
