#include "wizchip/udp.h"
#include "wizchip/udp/client.h"
#include "Ethernet/socket.h"
#include "etl/keywords.h"

using namespace Project::wizchip;


auto udp::request(etl::Vector<uint8_t> host, int port, etl::Vector<uint8_t> data) -> etl::Future<etl::Vector<uint8_t>> {
    return [host, port, data=etl::move(data)](etl::Time timeout) -> etl::Result<etl::Vector<uint8_t>, osStatus_t> {
        auto cli = udp::Client({.host=host, .port=port});
        return cli.request(etl::move(data)).wait(timeout);
    };
}

auto udp::transmit(etl::Vector<uint8_t> ip, int port, etl::Vector<uint8_t> data) -> etl::Result<void, osStatus_t> {
    auto s = SocketSession(Sn_MR_UDP, 0, ip, port);
    return detail::udp_transmit(s.socket_number, etl::move(ip), port, etl::move(data));
}

auto udp::receive(etl::Vector<uint8_t> ip) -> etl::Result<etl::Vector<uint8_t>, osStatus_t> {
    auto s = SocketSession(Sn_MR_UDP, 0, ip, 50000);
    return detail::udp_receive(s.socket_number, ip);
}
