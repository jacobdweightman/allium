#include <iostream>

#include "LSPServer/JSON.h"
#include "LSPServer/LSPServer.h"

int main() {
    LSPServer server(std::cin, std::cout);
    while(true) {
        server.serveNextRequest();
    }
}
