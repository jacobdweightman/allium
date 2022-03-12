#ifndef LSPSERVER_LSPSERVER_H
#define LSPSERVER_LSPSERVER_H

#include <iostream>
#include <fstream>
#include <map>

#include "JSON.h"
#include "LSPTypes.h"
#include "Utils/Optional.h"
#include "Utils/TaggedUnion.h"
#include "Utils/Unit.h"

class JSONRPCServer {
    std::istream &in;
    std::ostream &out;
    std::ofstream log;

    std::map<std::string, std::function<Optional<JSON>(JSON)>> handlers;

    Optional<int> parseContentLength();

    JSON makeErrorResponse(
        TaggedUnion<std::string, int, Unit> id,
        int code,
        std::string msg
    );

    JSON makeSuccessResponse(
        TaggedUnion<std::string, int, Unit> id,
        JSON result
    );

    JSON handle(JSON request);

public:
    JSONRPCServer(std::istream &in, std::ostream &out): in(in), out(out), log("ls-log.json") {}

    template <typename ParamType, typename ResponseType>
    bool registerHandler(std::string method, std::function<ResponseType(ParamType)> handler) {
        // Because the handler escapes, it must be captured by value.
        auto fullHandler = [=](JSON params) -> Optional<JSON> {
            return ParamType::decode(params)
                .template flatMap<JSON>([&](ParamType typedParams) {
                    return handler(typedParams).encode();
            });
        };

        auto result = handlers.emplace(method, fullHandler);
        return result.second;
    }

    void serveNextRequest();
};

class LSPServer : public JSONRPCServer {
    InitializeResult handleInitialize(InitializeParams params);

public:
    LSPServer(std::istream &in, std::ostream &out): JSONRPCServer(in, out) {
        using namespace std::placeholders;
        this->registerHandler<InitializeParams, InitializeResult>("initialize",  std::bind(&LSPServer::handleInitialize, this, _1));
    }
};

#endif // LSPSERVER_LSPSERVER_H
