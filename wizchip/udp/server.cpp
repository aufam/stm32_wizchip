#include "wizchip/udp/server.h"
#include "Ethernet/socket.h"
#include "etl/async.h"
#include "etl/keywords.h"

using namespace Project::wizchip;


int udp::Server::on_init(int socket_number) {
    return on_established(socket_number);
}

int udp::Server::on_listen(int socket_number) {
    return on_established(socket_number);
}

int udp::Server::on_established(int socket_number) {
    auto res = detail::udp_receive(socket_number, client_ip);
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
        res >> [this, socket_number](etl::Iter<const uint8_t*> data) {
            ::sendto(socket_number, const_cast<uint8_t*>(&(*data)), data.len(), client_ip.data(), port);
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

int udp::Server::on_close_wait(int socket_number) {
    return ::disconnect(socket_number);
}

int udp::Server::on_closed(int socket_number) {
    auto res = ::socket(socket_number, Sn_MR_UDP, port, 0);
    return res == socket_number ? SOCK_OK : res;
}