#include "Ethernet/socket.h"
#include "wizchip/http/server.h"
#include "etl/string.h"
#include <stdio.h>

using namespace Project;
using namespace Project::wizchip;

uint8_t http::Server::txData[WIZCHIP_BUFFER_LENGTH] = {};

static void status_stringify(http::Response& response) {
    if (response.status_string)
        return;

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

    for (int i = 0; i < handlers_cnt; ++i) {
        auto &handler = handlers[i];
        auto matches = request.url.match(handler.url);
        response.matches_ = &matches;

        if (request.method == handler.method and (matches.len() > 0 or request.url == handler.url)) {
            handler.function(request, response);
            if (response.status < 0)
                response.status = 200;
            break;
        }
    }

    if (response.status < 0) {
        const char static nf[] =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain"
            "\r\n\r\n"
            "Not Found";
        
        ::send(socket_number, (uint8_t*) nf, sizeof(nf));
        return;
    }

    status_stringify(response);

    auto &text = etl::string_cast(txData);
    text(
        "%.*s %d %s\r\n"
        "%s\r\n\r\n"
        "%s", 
        response.version.len(), response.version.data(), response.status, response.status_string,
        response.head,
        response.body
    );
    ::send(socket_number, txData, text.len());
}

void http::Server::add(Handler handler) {
    handlers[handlers_cnt++] = handler;
}

void http::Server::listen() {
    ethernet.registerSocket(this, 4);
}

void http::Server::stop() {
    ethernet.unregisterSocket(this);
}