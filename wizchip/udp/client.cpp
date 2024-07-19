#include "Ethernet/socket.h"
#include "wizchip/udp/client.h"
#include "etl/keywords.h"

using namespace wizchip;

auto udp::Client::request(Stream s) -> etl::Future<etl::Vector<uint8_t>> {
    return [this, s=mv | s](etl::Time timeout) mutable -> etl::Result<etl::Vector<uint8_t>, osStatus_t> {
        s >> [this](etl::Iter<const uint8_t*> data) {
            ::sendto(socket_number, const_cast<uint8_t*>(&(*data)), data.len(), host.data(), port);
        };
        
        size_t retry = timeout.tick;
        while (true) {
            auto r = detail::udp_receive(this->socket_number, this->host);
            if (r.is_ok()) return r;

            retry--;
            if (retry == 0) break;
            etl::this_thread::sleep(1ms);
        }

        return etl::Err(osErrorTimeout);
    };
}

auto udp::request(etl::Vector<uint8_t> host, int port, Stream data) -> etl::Future<etl::Vector<uint8_t>> {
    return [host, port, data=mv | data](etl::Time timeout) mutable -> etl::Result<etl::Vector<uint8_t>, osStatus_t> {
        auto cli = udp::Client({.host=host, .port=port});
        return cli.request(mv | data).wait(timeout);
    };
}