#ifndef WIZCHIP_TCP_CLIENT_H
#define WIZCHIP_TCP_CLIENT_H

#include "wizchip/ethernet.h"
#include "wizchip/url.h"

namespace Project::wizchip::tcp {
    class Client : public SocketSession {
    public:
        struct Args {
            etl::Vector<uint8_t> host;
            int port;
        };

        Client(Args args) : SocketSession(Sn_MR_TCP, 0, args.host, args.port) {}

        etl::Future<etl::Vector<uint8_t>> request(etl::Vector<uint8_t> data);
    };

    etl::Future<etl::Vector<uint8_t>> request(etl::Vector<uint8_t> host, int port, etl::Vector<uint8_t> data);
} 

#endif // WIZCHIP_TCP_CLIENT_H