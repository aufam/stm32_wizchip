#include "Ethernet/socket.h"
#include "wizchip/tcp/client.h"
#include "etl/heap.h"
#include "etl/keywords.h"

using namespace Project::wizchip;

auto tcp::Client::request(Stream s) -> etl::Future<etl::Vector<uint8_t>> {
    return [this, s=mv | s](etl::Time timeout) mutable -> etl::Result<etl::Vector<uint8_t>, osStatus_t> {
        auto start_time = etl::time::now();

        for (; etl::time::elapsed(start_time) < timeout;) {
            if (::connect(socket_number, host.data(), port) == SOCK_OK) break; 
            etl::this_thread::sleep(1ms);
        }

        if (etl::time::elapsed(start_time) >= timeout) {
            return etl::Err(osErrorTimeout);
        }

        s >> [this](etl::Iter<const uint8_t*> data) {
            ::send(socket_number, const_cast<uint8_t*>(&(*data)), data.len());
        };

        for (; etl::time::elapsed(start_time) < timeout;) {
            auto r = detail::tcp_receive(socket_number);
            if (r.is_ok()) return r;
            
            etl::this_thread::sleep(1ms);
        }
        
        return etl::Err(osErrorTimeout);
    };
}
