#include "Ethernet/socket.h"
#include "wizchip/http/server.h"
#include "etl/string.h"
#include <stdio.h>

using namespace Project;
using namespace Project::wizchip;

static void status_stringify(http::Response& response) {
    switch (response.status) {
        default: response.status_string = ""; break;
        case 200: response.status_string = "OK"; break;
        case 201: response.status_string = "Created"; break;
        case 204: response.status_string = "No Content"; break;
        case 205: response.status_string = "Bad Request"; break;
        case 401: response.status_string = "Unauthorized"; break;
        case 403: response.status_string = "Forbidden"; break;
        case 404: response.status_string = "Not Found"; break;
        case 500: response.status_string = "Internal Server Error"; break;
    }
}

void http::Server::process(int socket_number, const uint8_t* rxBuffer, size_t len) {
    auto request = Request::parse(rxBuffer, len);
    auto response = Response {};
    response.version = request.version;

    // handling
    bool handled = false;
    for (int i = 0; i < handlers_cnt; ++i) {
        auto &handler = handlers[i];
        auto matches = request.url.match(handler.url);
        request.matches_ = &matches;

        if (request.method == handler.method and (matches.len() > 0 or request.url == handler.url)) {
            handler.function(request, response);
            handled = true;            
            break;
        }
    }

    // send not found message if not handled
    if (not handled) {
        const char static nf[] =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "Not Found";
        
        ::send(socket_number, (uint8_t*) nf, sizeof(nf));
        return;
    }

    // compile all
    auto &text = etl::string_cast(Ethernet::txData);

    // create status
    if (response.status != 200 and response.status_string == "OK") // modified by handler
        status_stringify(response);
    
    text("%.*s %d %.*s\r\n", 
        response.version.len(), response.version.data(), 
        response.status, 
        response.status_string.len(), response.status_string.data()
    );

    // create head
    bool head_is_empty = true;
    for (auto &[key, value] : response.head) if (key and value) {
        strncat(text.data(), key.data(), key.len());
        text += ": ";
        strncat(text.data(), value.data(), value.len());
        text += "\r\n";
        head_is_empty = false;
    }

    if (head_is_empty) {
        sprintf(text.end(), "Content-Length: %d\r\n", response.body.len());
    }

    // create body
    sprintf(text.end(), "\r\n%.*s", response.body.len(), response.body.data());

    // send
    ::send(socket_number, Ethernet::txData, text.len());
}

auto http::Server::add(Handler handler) -> http::Server& {
    handlers[handlers_cnt++] = handler;
    return *this;
}

auto http::Server::start() -> http::Server& {
    ethernet.registerSocket(this, _number_of_socket);
    return *this;
}

auto http::Server::stop() -> http::Server& {
    ethernet.unregisterSocket(this);
    return *this;
}