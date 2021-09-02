#include <iostream>

#include "JSON.h"
#include "LSPServer.h"

int main() {
    LSPServer server(std::cin, std::cout);
    while(true) {
        server.serveNextRequest();
    }
}
