#include "Ethernet/socket.h"
#include "wizchip/http/client.h"
#include "etl/string.h"

using namespace Project;
using namespace Project::wizchip;

int http::Client::on_init(int) {
    return SOCK_OK;
}

int http::Client::on_listen(int) {
    return SOCK_OK;
}

int http::Client::on_established(int socket_number) {
    // Interrupt clear
    if (getSn_IR(socket_number) & Sn_IR_CON)
        setSn_IR(socket_number, Sn_IR_CON);

    if (state == STATE_START) {
        // send request
        this->process(socket_number, Ethernet::txData);

        // Check the TX socket buffer for End of HTTP response sends
        while (getSn_TX_FSR(socket_number) != (getSn_TxMAX(socket_number))) {}

        state = STATE_ESTABLISHED;
    }

    if (state == STATE_ESTABLISHED) {
        // receive response
        size_t len = getSn_RX_RSR(socket_number);
        
        if (len > WIZCHIP_BUFFER_LENGTH) 
            len = WIZCHIP_BUFFER_LENGTH;

        len = ::recv(socket_number, Ethernet::rxData, len);
        Ethernet::rxData[len] = '\0';

        _response = Response::parse(Ethernet::rxData, len);
        if (handler) {
            handler(_response);
        } else {
            event.setFlags(1);
        }
        state = STATE_STOP;
    }

    return SOCK_OK;
}

int http::Client::on_close_wait(int socket_number) {
    return ::disconnect(socket_number);
}

int http::Client::on_closed(int socket_number) {
    if (state == STATE_START) {
        auto ip = host.split<5>(".");
        uint8_t addr[] = { (uint8_t) ip[0].to_int(), (uint8_t) ip[1].to_int(), (uint8_t) ip[2].to_int(), 247 };
        ::socket(socket_number, Sn_MR_TCP, port, Sn_MR_ND);
        return ::connect(socket_number, addr, port);
    }
    if (state == STATE_STOP) {
        deallocate(socket_number);
        return SOCK_OK;
    }
    else { // TODO: ??
        return SOCK_OK;
    }
}

auto http::Client::request(const http::Request& req) -> http::Response {
    _request = req;
    this->handler = nullptr;
    state = STATE_START;
    allocate(1);

    event.init();
    event.resetFlags(1);

    if (event.waitFlagsAny({.timeout=_timeout})) {
        return _response;
    } else {
        return {};
    }
}

void http::Client::request(const http::Request& req, HandlerFunction handler) {
    _request = req;
    this->handler = handler;
    state = STATE_START;
    allocate(1);
}

void http::Client::process(int socket_number, uint8_t* txBuffer) {
    auto &text = etl::string_cast<WIZCHIP_BUFFER_LENGTH>(txBuffer);
    text(
        "%.*s %.*s %.*s" "\r\n"
        "%.*s" "\r\n\r\n"
        "%.*s",
        _request.method.len(), _request.method.begin(), 
        _request.url.len(), _request.url.begin(),
        _request.version.len(), _request.version.begin(),
        _request.head.len(), _request.head.begin(),
        _request.body.len(), _request.body.begin()
    );

    // send
    ::send(socket_number, Ethernet::txData, text.len());
}