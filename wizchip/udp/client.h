#ifndef WIZCHIP_UDP_CLIENT_H
#define WIZCHIP_UDP_CLIENT_H

#include "wizchip/ethernet.h"
#include "etl/vector.h"
#include "etl/future.h"

namespace Project::wizchip::udp {
    class Client : public SocketSession {
    public:
        struct Args {
            etl::Vector<uint8_t> host;
            int port;
        };

        Client(Args args) : SocketSession(Sn_MR_UDP, 0, args.host, args.port) {}

        etl::Future<etl::Vector<uint8_t>> request(Stream data) override;
    };

    etl::Future<etl::Vector<uint8_t>> request(etl::Vector<uint8_t> host, int port, Stream data);
}

#endif // WIZCHIP_UDP_CLIENT_H