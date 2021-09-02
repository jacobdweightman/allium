#include <math.h>

#include "LSPServer/LSPTypes.h"
#include "Utils/VectorUtils.h"

// This is a trick to pass types with multiple template parameters to a macro.
// Extra parentheses are allowed around function arguments, so these templates
// make it possible to pass a parenthesized template type to a macro and then
// strip them off in the template definition.
//
// Credit: https://stackoverflow.com/a/13842784/15786199
template<typename T> struct argument_type;
template<typename T, typename U> struct argument_type<T(U)> { typedef U type; };


// Decoding support

#define DECODE_PROPERTY_WITH_DECODER(OBJECT, PROPERTY_NAME, TYPE, DECODER) \
    Optional<argument_type<void(TYPE)>::type> PROPERTY_NAME = OBJECT \
        .getAtKey(#PROPERTY_NAME) \
        .flatMap<argument_type<void(TYPE)>::type>([](JSON v) { return DECODER(v); });

#define DECODE_PROPERTY(OBJECT, PROPERTY_NAME, TYPE) \
    DECODE_PROPERTY_WITH_DECODER(OBJECT, PROPERTY_NAME, TYPE, TYPE::decode)

#define DECODE_PRIMITIVE_PROPERTY(OBJECT, PROPERTY_NAME, TYPE) \
    DECODE_PROPERTY_WITH_DECODER(OBJECT, PROPERTY_NAME, TYPE, decodeJSON<TYPE>)


// Decoding support

template <>
Optional<Unit> decodeJSON(JSON json) {
    return json.as_a<Unit>();
}

template <>
Optional<bool> decodeJSON(JSON json) {
    return json.as_a<bool>();
}

template <>
Optional<double> decodeJSON(JSON json) {
    return json.as_a<double>();
}

/// Note: this will behave badly for doubles that do not store their 1's bit.
template <>
Optional<int> decodeJSON(JSON json) {
    return json.as_a<double>().flatMap<int>([](double d) -> Optional<int> {
        if(trunc(d) == d) {
            return (int) d;
        } else {
            return Optional<int>();
        }
    });
}

template <>
Optional<std::string> decodeJSON(JSON json) {
    return json.as_a<std::string>();
}

// Encoding support

JSON encodeJSON(Unit) {
    return JSON(Unit());
}

JSON encodeJSON(bool b) {
    return JSON(b);
}

JSON encodeJSON(double d) {
    return JSON(d);
}

JSON encodeJSON(int i) {
    return JSON((double) i);
}

JSON encodeJSON(std::string str) {
    return JSON(str);
}

Optional<JSONRPCRequest> JSONRPCRequest::decode(JSON json) {
    return json.as_a<JSONObject>().flatMap<JSONRPCRequest>([](JSONObject o) {
        DECODE_PRIMITIVE_PROPERTY(o, jsonrpc, std::string);
        DECODE_PRIMITIVE_PROPERTY(o, method, std::string);
        Optional<JSON> params = o.getAtKey("params");
        DECODE_PROPERTY_WITH_DECODER(o, id, (TaggedUnion<std::string, int, Unit>), (decodeJSONUnion<std::string, int, Unit>));

        return all<std::string, std::string, JSON, TaggedUnion<std::string, int, Unit>>(
            jsonrpc, method, params, id
        ).map<JSONRPCRequest>([&](auto properties) {
            return JSONRPCRequest(
                std::get<0>(properties),
                std::get<1>(properties),
                std::get<2>(properties),
                std::get<3>(properties));
        });
    });
}

Optional<ClientInfo> ClientInfo::decode(JSON json) {
    return json.as_a<JSONObject>().flatMap<ClientInfo>([](JSONObject o) {
        DECODE_PRIMITIVE_PROPERTY(o, name, std::string);
        DECODE_PRIMITIVE_PROPERTY(o, version, std::string);

        return name.map<ClientInfo>([&](std::string name) {
            return ClientInfo(
                name,
                version);
        });
    });
}

JSON ClientInfo::encode() {
    JSONObject o = {
        { "name", encodeJSON(name) }
    };
    version.then([&](std::string version) { o.emplace("version", encodeJSON(version)); });
    return JSON(o);
}

bool operator==(const ClientInfo &lhs, const ClientInfo &rhs) {
    return lhs.name == rhs.name && lhs.version == rhs.version;
}

Optional<InitializeParams> InitializeParams::decode(JSON json) {
    return json.as_a<JSONObject>().flatMap<InitializeParams>([](JSONObject o) {
        DECODE_PROPERTY_WITH_DECODER(o, processId, (TaggedUnion<int, Unit>), (decodeJSONUnion<int, Unit>));
        DECODE_PROPERTY(o, clientInfo, ClientInfo);

        return all<TaggedUnion<int, Unit>>(
            processId
        ).map<InitializeParams>([&](auto properties) {
            return InitializeParams(
                std::get<0>(properties),
                clientInfo);
        });
    });
}

bool operator==(const InitializeParams &lhs, const InitializeParams &rhs) {
    return lhs.processId == rhs.processId && lhs.clientInfo == rhs.clientInfo;
}

JSON SemanticTokensLegend::encode() {
    return JSON(JSONObject({
        { "tokenTypes", encodeJSON(tokenTypes) },
        { "tokenModifiers", encodeJSON(tokenModifiers) }
    }));
}

bool operator==(const SemanticTokensLegend &lhs, const SemanticTokensLegend &rhs) {
    return lhs.tokenTypes == rhs.tokenTypes &&
        lhs.tokenModifiers == rhs.tokenModifiers;
}

JSON SemanticTokensOptions::encode() {
    JSONObject o = {
        { "legend", legend.encode() }
    };
    range.then([&](bool b) { o.emplace("range", b); });
    full.then([&](bool b) { o.emplace("full", b); });
    return JSON(o);
}

bool operator==(const SemanticTokensOptions &lhs, const SemanticTokensOptions &rhs) {
    return lhs.legend == rhs.legend && lhs.range == rhs.range &&
        lhs.full == rhs.full;
}

JSON ServerCapabilities::encode() {
    JSONObject o = {};
    semanticTokensProvider.then([&](auto stp) {
        o.emplace("semanticTokensProvider", stp.encode());
    });
    return JSON(o);
}

bool operator==(const ServerCapabilities &lhs, const ServerCapabilities &rhs) {
    assert(false);
    return true;
}

JSON InitializeResult::encode() {
    JSONObject o = {
        { "capabilities", capabilities.encode() }
    };
    serverInfo.then([&](ClientInfo serverInfo) {
        o.emplace("serverInfo", serverInfo.encode());
    });
    return JSON(o);
}

bool operator==(const InitializeResult &lhs, const InitializeResult & rhs) {
    assert(false);
    return true;
}
