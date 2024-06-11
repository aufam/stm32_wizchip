#ifndef WIZCHIP_UDP_H
#define WIZCHIP_UDP_H

#include "etl/vector.h"
#include "etl/future.h"

namespace Project::wizchip::udp {
    etl::Result<void, osStatus_t> transmit(etl::Vector<uint8_t> host, int port, etl::Vector<uint8_t> data);
    etl::Result<etl::Vector<uint8_t>, osStatus_t> receive(etl::Vector<uint8_t> host);
    etl::Future<etl::Vector<uint8_t>> request(etl::Vector<uint8_t> host, int port, etl::Vector<uint8_t> data);
}

#endif // WIZCHIP_UDP_H