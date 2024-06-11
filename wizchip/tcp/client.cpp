#include "Ethernet/socket.h"
#include "wizchip/tcp/client.h"
#include "etl/keywords.h"

using namespace Project::wizchip;


auto tcp::Client::request(etl::Vector<uint8_t> data) -> etl::Future<etl::Vector<uint8_t>> {
    return [this, data=etl::move(data)](etl::Time timeout) mutable -> etl::Result<etl::Vector<uint8_t>, osStatus_t> {
        size_t retry = 0;
        for (; retry < timeout.tick; ++retry) {
            if (::connect(socket_number, host.data(), port) == SOCK_OK) break; 
            etl::this_thread::sleep(1ms);
        }

        if (retry == timeout.tick) {
            return etl::Err(osErrorTimeout);
        }

        auto t = detail::tcp_transmit(socket_number, etl::move(data));
        if (t.is_err()) {
            return etl::Err(t.unwrap_err());
        }

        for (; retry < timeout.tick; ++retry) {
            auto r = detail::tcp_receive(socket_number);
            if (r.is_ok()) return r;
            
            etl::this_thread::sleep(1ms);
        }
        
        return etl::Err(osErrorTimeout);
    };
}


auto tcp::request(etl::Vector<uint8_t> host, int port, etl::Vector<uint8_t> data) -> etl::Future<etl::Vector<uint8_t>> {
    return [host, port, data=etl::move(data)](etl::Time timeout) mutable -> etl::Result<etl::Vector<uint8_t>, osStatus_t> {
        auto cli = tcp::Client({.host=host, .port=port});
        return cli.request(etl::move(data)).wait(timeout);
    };
}