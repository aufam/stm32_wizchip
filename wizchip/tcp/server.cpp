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
    if (res.is_err())
        return SOCK_OK;

    auto future = etl::async([this, socket_number, data=etl::move(res.unwrap())]() mutable {
        auto res = this->response(etl::move(data));
        auto lock = Ethernet::self->mutex.lock().await();
        detail::tcp_transmit(socket_number, etl::move(res));
        ::disconnect(socket_number);
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
