#include "wizchip/tcp/server.h"
#include "Ethernet/socket.h"
#include "etl/async.h"
#include "etl/keywords.h"

using namespace Project::wizchip;


int tcp::Server::on_init(int socket_number) {
    return ::listen(socket_number);
}

int tcp::Server::on_listen(int) {
    return SOCK_OK;
}

int tcp::Server::on_established(int socket_number) {
    auto res = detail::tcp_receive(socket_number);
    if (res.is_err()) {
        auto err = res.unwrap_err();
        if (err == osErrorNoMemory) {
            return SOCKERR_BUFFER;
        } else {
            return SOCK_OK;
        }
    }

    auto future = etl::async([this, socket_number, data=etl::move(res.unwrap())]() mutable {
        auto res = this->response(socket_number, etl::move(data));
        auto lock = Ethernet::self->mutex.lock().await();
        res >> [socket_number](etl::Iter<const uint8_t*> data) {
            ::send(socket_number, const_cast<uint8_t*>(&(*data)), data.len());
        };
        // ::disconnect(socket_number);
    });
    
    // no thread available
    if (not future.valid()) {
        ::disconnect(socket_number);
        return SOCK_ERROR;
    }

    return SOCK_OK;
}

int tcp::Server::on_close_wait(int socket_number) {
    return ::disconnect(socket_number);
}

int tcp::Server::on_closed(int socket_number) {
    auto res = ::socket(socket_number, Sn_MR_TCP, port, Sn_MR_ND);
    return res == socket_number ? SOCK_OK : res;
}
