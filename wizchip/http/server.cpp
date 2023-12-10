#include "Ethernet/socket.h"
#include "wizchip/http/server.h"
#include "etl/string.h"
#include <stdio.h>

using namespace Project;
using namespace Project::wizchip;

int http::Server::on_init(int socket_number) {
    return ::listen(socket_number);
}

int http::Server::on_listen(int) {
    return SOCK_OK;
}

int http::Server::on_established(int socket_number) {
    // Interrupt clear
    if (getSn_IR(socket_number) & Sn_IR_CON)
        setSn_IR(socket_number, Sn_IR_CON);

    size_t len = getSn_RX_RSR(socket_number);
    if (len == 0) 
        return SOCK_ERROR;
    
    if (len > WIZCHIP_BUFFER_LENGTH) 
        len = WIZCHIP_BUFFER_LENGTH;

    len = ::recv(socket_number, Ethernet::rxData, len);
    Ethernet::rxData[len] = '\0';

    this->process(socket_number, Ethernet::rxData, len);

    // Check the TX socket buffer for End of HTTP response sends
    while (getSn_TX_FSR(socket_number) != (getSn_TxMAX(socket_number))) {}

    return SOCK_OK;
}

int http::Server::on_close_wait(int socket_number) {
    return ::disconnect(socket_number);
}

int http::Server::on_closed(int socket_number) {
    if (_is_running) {
        // init socket again
        return ::socket(socket_number, Sn_MR_TCP, port, 0x00);
    } else {
        deallocate(socket_number);
    }
    return SOCK_OK;
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
    if (response.status != 200 and response.status_string == "OK") { // modified by user
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
    
    text("%.*s %d %.*s\r\n", 
        response.version.len(), response.version.data(), 
        response.status, 
        response.status_string.len(), response.status_string.data()
    );

    // create head
    if (response.head) {
        ::sprintf(text.end(), "%.*s\r\n", response.head.len(), response.head.data());
    } else {
        auto begin = text.end();
        ::sprintf(begin, "Content-Length: %d\r\n", response.body.len());
        response.head = begin;
    }

    // create body
    ::sprintf(text.end(), "\r\n%.*s", response.body.len(), response.body.data());

    // send
    ::send(socket_number, Ethernet::txData, text.len());

    // debug message
    debug(request, response);
}

auto http::Server::add(Handler handler) -> http::Server& {
    handlers[handlers_cnt++] = handler;
    return *this;
}

auto http::Server::start() -> http::Server& {
    _is_running = true;
    allocate(_number_of_socket);
    return *this;
}

auto http::Server::stop() -> http::Server& {
    _is_running = false;
    return *this;
}