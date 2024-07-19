#include "wizchip/dns.h"
#include "wizchip/udp/client.h"
#include "etl/keywords.h"

using namespace wizchip;

static auto make_query(const std::string& domain) -> Stream {
    static const uint8_t start_bytes[] = {
        0x11, 0x22, // message id
        0x01, 0x00, // recursion desired
        0x00, 0x01,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
    };

    static const uint8_t stop_bytes[] = {
        0x00, // null terminator
        0x00, 0x01, // type
        0x00, 0x01, // class
    };

    auto labels = etl::string_view(domain.c_str()).split<8>(".");
    auto data = etl::vector_reserve<uint8_t>(labels.len() + domain.size());

    for (auto label in labels) {
        data.append(label.len());
        for (auto ch in label) {
            data.append(ch);
        }
    }

    Stream s;
    s << etl::iter(start_bytes) << [data=mv | data]() { return data.iter(); } << etl::iter(stop_bytes);

    return s;
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

auto dns::get_ip(const std::string& domain) -> etl::Future<etl::Vector<uint8_t>> {
    return udp::request(etl::vectorize<uint8_t>(Ethernet::self->getNetInfo().dns), 53, make_query(domain))
        .and_then([](etl::Vector<uint8_t> received_data) {
            return parse_data(received_data);
        });
}