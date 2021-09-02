#include "LSPServer/LSPServer.h"

Optional<int> JSONRPCServer::parseContentLength() {
    std::string is;
    if(std::getline(in, is)) {
        std::stringstream iss(is);
        std::string headerField;
        iss >> headerField;
        assert(headerField == "Content-Length:");

        int length;
        iss >> length;
        return length;
    } else {
        return Optional<int>();
    }
}

JSON JSONRPCServer::makeErrorResponse(
    TaggedUnion<std::string, int, Unit> id,
    int code,
    std::string msg
) {
    JSON jsonID = encodeJSON<std::string, int, Unit>(id);

    return JSON(JSONObject({
        { "jsonrpc", JSON("2.0") },
        { "id", jsonID },
        { "error", JSON(JSONObject({
            { "code", JSON((double) code) },
            { "message", JSON(msg) },
            { "data", JSON(JSONObject()) }
        })) }
    }));
}

JSON JSONRPCServer::makeSuccessResponse(
    TaggedUnion<std::string, int, Unit> id,
    JSON result
) {
    JSON jsonID = encodeJSON<std::string, int, Unit>(id);

    return JSON(JSONObject({
        { "jsonrpc", JSON("2.0") },
        { "id", jsonID },
        { "result", result }
    }));
}

JSON JSONRPCServer::handle(JSON request) {
    request.serialize(log);
    log << std::endl;

    return JSONRPCRequest::decode(request)
        .map<JSON>([&](JSONRPCRequest request) {
            auto handler = handlers.find(request.method);
            if(handler == handlers.end()) {
                return makeErrorResponse(request.id, -32601, "Method not found");
            }
            return handler->second(request.params).map<JSON>([&](JSON result) {
                return this->makeSuccessResponse(request.id, result);
            }).coalesce(
                makeErrorResponse(request.id, -32000, "Error in handler")
            );
        }).coalesce(
            // If there was an error in detecting the id in the Request
            // object (e.g. Parse error/Invalid Request), it MUST be Null.
            makeErrorResponse(
                TaggedUnion<std::string, int, Unit>(Unit()),
                -32700,
                "Parse error")
        );
}

void JSONRPCServer::serveNextRequest() {
    parseContentLength().error([&]() {
        out << "error parsing content length\n";
    }).map([&](int length) {
        in.get(); // '\r'
        in.get(); // '\n'

        std::stringstream body;
        for(int i=0; i<length; ++i) {
            body << ((char) in.get());
        }

        // TODO: request is not valid JSON?
        JSON::parse(body).error([&]() {
            out << "error parsing JSON\n";
        }).map([&](JSON request) {
            JSON result = handle(request);
            std::stringstream buffer;
            result.serialize(buffer);
            out << "Content-Length: " << buffer.str().size() << "\r\n\r\n" <<
                buffer.str();

            handle(request).serialize(log);
            log << std::endl;
        });
    });
    out << "\r\n";
}

InitializeResult LSPServer::handleInitialize(InitializeParams params) {
    return InitializeResult(
        ServerCapabilities(
            SemanticTokensOptions(
                SemanticTokensLegend(
                    {"enumMember", "variable"},
                    {}
                ),
                true,
                true
            )
        ),
        ClientInfo(
            std::string("allium-lsp"),
            std::string("0.0.1")
        )
    );
}
