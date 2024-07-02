#ifndef WIZCHIP_UDP_SERVER_H
#define WIZCHIP_UDP_SERVER_H

#include "wizchip/ethernet.h"
#include "etl/vector.h"
#include "etl/future.h"

namespace Project::wizchip::udp {
    class Server : public SocketServer {
    public:
        etl::Vector<uint8_t> client_ip;

    protected:
        int on_init(int socket_number) override;
        int on_listen(int socket_number) override;
        int on_established(int socket_number) override;
        int on_close_wait(int socket_number) override;
        int on_closed(int socket_number) override;
        const char* kind() override { return "UDP"; }
    };
}

#endif // WIZCHIP_UDP_SERVER_H