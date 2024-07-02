#ifndef WIZCHIP_TCP_SERVER_H
#define WIZCHIP_TCP_SERVER_H

#include "wizchip/ethernet.h"

namespace Project::wizchip::tcp {
    class Server : public SocketServer {
    protected:
        int on_init(int socket_number) override;
        int on_listen(int socket_number) override;
        int on_established(int socket_number) override;
        int on_close_wait(int socket_number) override;
        int on_closed(int socket_number) override;
        const char* kind() override { return "TCP"; }
    };
} 

#endif // WIZCHIP_TCP_SERVER_H