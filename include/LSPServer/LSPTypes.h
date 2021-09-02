#ifndef LSPTYPES_H
#define LSPTYPES_H

#include <string>

#include "LSPServer/JSON.h"
#include "Utils/Optional.h"
#include "Utils/TaggedUnion.h"
#include "Utils/Unit.h"
#include "Utils/VectorUtils.h"

/// An interface which represents a type which can be decoded from JSON.
/// Use CRTP to impose "self" requirement.
template <typename T>
struct Decodable {
    static Optional<T> decode(JSON json);
};

/**
 * decodeJSON implements decoding from JSON for various "currency" types.
 * 
 * To decode param/result types defined by the Language Server Protocol, use the
 * corresponding type's static `decode` method.
 */
template <typename T>
Optional<T> decodeJSON(JSON);

template <typename T>
Optional<std::vector<T>> decodeJSONArray(JSON json) {
    return json.as_a<JSONArray>().flatMap<std::vector<T>>([](JSONArray arr) {
        return fullMap<JSON, T>(arr, [](JSON e) {
            return decodeJSON<T>(e);
        });
    });
}

template <typename ...Ts>
Optional<TaggedUnion<Ts...>> decodeJSONUnion(JSON json) {
    return decodeJSONUnionHelper<0, Ts...>(json);
}

template <size_t N, typename ...Ts>
Optional<TaggedUnion<Ts...>> decodeJSONUnionHelper(JSON json) {
    using type_n = typename std::tuple_element<N, std::tuple<Ts...>>::type;
    type_n t;

    if(decodeJSON<type_n>(json).unwrapInto(t)) {
        return TaggedUnion<Ts...>(t);
    }
    if constexpr (N + 1 < sizeof...(Ts)) {
        return decodeJSONUnionHelper<N+1, Ts...>(json);
    }
    return Optional<TaggedUnion<Ts...>>();
}

/// An interface which represents a type which can be encoded as JSON.
struct Encodable {
    virtual JSON encode() = 0;
};

JSON encodeJSON(Unit);
JSON encodeJSON(bool);
JSON encodeJSON(double);
JSON encodeJSON(int);
JSON encodeJSON(std::string);

template <typename T>
JSON encodeJSON(std::vector<T> arr) {
    return JSON(map<T, JSON>(
        arr,
        [](std::string str) { return encodeJSON(str); }
    ));
}

template <typename ...Ts>
JSON encodeJSON(TaggedUnion<Ts...> tu) {
    return encodeJSONUnionHelper<0, Ts...>(tu);
}

template <size_t N, typename ...Ts>
JSON encodeJSONUnionHelper(TaggedUnion<Ts...> tu) {
    using type_n = typename std::tuple_element<N, std::tuple<Ts...>>::type;
    type_n t;

    if(tu.template as_a<type_n>().unwrapInto(t)) {
        return encodeJSON(t);
    }
    if constexpr (N + 1 < sizeof...(Ts)) {
        return encodeJSONUnionHelper<N+1, Ts...>(tu);
    }
    throw "impossible!";
}

/**
 * Language Server Protocol types
 */

struct JSONRPCRequest : public Decodable<JSONRPCRequest> {
    std::string jsonrpc;
    std::string method;
    JSON params;
    TaggedUnion<std::string, int, Unit> id;

    JSONRPCRequest(
        std::string jsonrpc,
        std::string method,
        JSON params,
        TaggedUnion<std::string, int, Unit> id
    ): jsonrpc(jsonrpc), method(method), params(params), id(id) {}

    static Optional<JSONRPCRequest> decode(JSON json);
};

struct ClientInfo : public Decodable<ClientInfo>, public Encodable {
    std::string name;
    Optional<std::string> version;

    ClientInfo(
        std::string name,
        Optional<std::string> version
    ): name(name), version(version) {}

    static Optional<ClientInfo> decode(JSON json);
    JSON encode();
};

bool operator==(const ClientInfo &lhs, const ClientInfo &rhs);

struct InitializeParams : public Decodable<InitializeParams> {
    TaggedUnion<int, Unit> processId;
    Optional<ClientInfo> clientInfo;

    InitializeParams(
        TaggedUnion<int, Unit> processId,
        Optional<ClientInfo> clientInfo
    ): processId(processId), clientInfo(clientInfo) {}

    static Optional<InitializeParams> decode(JSON json);
};

bool operator==(const InitializeParams &lhs, const InitializeParams &rhs);

struct SemanticTokensLegend : public Encodable {
    /// The token types a server uses.
    std::vector<std::string> tokenTypes;

    /// The token modifiers a server uses.
    std::vector<std::string> tokenModifiers;

    SemanticTokensLegend(
        std::vector<std::string> tokenTypes,
        std::vector<std::string> tokenModifiers
    ): tokenTypes(tokenTypes), tokenModifiers(tokenModifiers) {}

    JSON encode();
};

bool operator==(const SemanticTokensLegend &lhs, const SemanticTokensLegend &rhs);

struct SemanticTokensOptions : public Encodable {
    /// The legend used by the server.
    SemanticTokensLegend legend;

    /// Server supports providing semantic tokens for a specific range of a
    /// document.
    Optional<bool> range;

    /// Server supports providing semantic tokens for a full document.
    Optional<bool> full;

    SemanticTokensOptions(
        SemanticTokensLegend legend,
        Optional<bool> range,
        Optional<bool> full
    ): legend(legend), range(range), full(full) {}

    JSON encode();
};

bool operator==(const SemanticTokensOptions &lhs, const SemanticTokensOptions &rhs);

struct ServerCapabilities : public Encodable {
    Optional<SemanticTokensOptions> semanticTokensProvider;

    ServerCapabilities(
        Optional<SemanticTokensOptions> semanticTokensProvider
    ): semanticTokensProvider(semanticTokensProvider) {}

    JSON encode();
};

bool operator==(const ServerCapabilities &lhs, const ServerCapabilities &rhs);

struct InitializeResult : public Encodable {
    ServerCapabilities capabilities;
    Optional<ClientInfo> serverInfo;

    InitializeResult(
        ServerCapabilities capabilities,
        Optional<ClientInfo> serverInfo
    ): capabilities(capabilities), serverInfo(serverInfo) {}

    JSON encode();
};

bool operator==(const InitializeResult &lhs, const InitializeResult & rhs);

#endif // LSPTYPES_H
