#include "wizchip/dns.h"
#include "wizchip/udp.h"
#include "etl/keywords.h"

using namespace Project::wizchip;


static auto make_query(const std::string& domain) -> etl::Vector<uint8_t> {
    auto labels = etl::string_view(domain.c_str()).split<8>(".");
    auto res = etl::vector_reserve<uint8_t>(12 + domain.size() + 2 + 4);

    // message id
    res.append(0x11);
    res.append(0x22);

    // recursion desired
    res.append(0x01);
    res.append(0x00);

    res.append(0x00);
    res.append(0x01);
    
    res.append(0x00);
    res.append(0x00);
    
    res.append(0x00);
    res.append(0x00);
    
    res.append(0x00);
    res.append(0x00);

    for (auto label in labels) {
        res.append(label.len());
        for (auto ch in label) {
            res.append(ch);
        }
    }

    res.append(0x00); // null terminator

    // type
    res.append(0x00);
    res.append(0x01); 

    // class
    res.append(0x00);
    res.append(0x01); 

    return res;
}

static auto parse_data(const etl::Vector<uint8_t>& message) -> etl::Result<etl::Vector<uint8_t>, osStatus_t> {
    for (int i = 12; i < int(message.len()) - 13; ++i) {
        bool check = 
            message[i + 0] == 0 and message[i + 1] == 1 and // type 
            // message[i + 2] == 0 and message[i + 3] == 1 and // class
            // message[i + 4] ... message[i + 7] // ttl
            message[i + 8] == 0 and message[i + 9] == 4; // length

        if (check) {
            return etl::Ok(etl::vector(message[i + 10], message[i + 11], message[i + 12], message[i + 13]));
        }
    }

    return etl::Err(osErrorNoMemory);
}

auto dns::get_ip(std::string domain) -> etl::Future<etl::Vector<uint8_t>> {
    return udp::request(etl::vectorize<uint8_t>(Ethernet::self->getNetInfo().dns), 53, make_query(domain))
        .and_then([](etl::Vector<uint8_t> received_data) {
            return parse_data(received_data);
        });
}