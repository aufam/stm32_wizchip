#include "wizchip/udp/client.h"
#include "etl/keywords.h"

using namespace Project::wizchip;


auto udp::Client::request(etl::Vector<uint8_t> data) -> etl::Future<etl::Vector<uint8_t>> {
    return [this, data=etl::move(data)](etl::Time timeout) -> etl::Result<etl::Vector<uint8_t>, osStatus_t> {
        auto t = detail::udp_transmit(this->socket_number, this->host, this->port, etl::move(data));
        if (t.is_err()) 
            return etl::Err(t.unwrap_err());
        
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
