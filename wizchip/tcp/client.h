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
        etl::Future<etl::Vector<uint8_t>> request(Stream s) override;
        etl::Future<Stream> request_test(Stream s);
    };
} 

#endif // WIZCHIP_TCP_CLIENT_H