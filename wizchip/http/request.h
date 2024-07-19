#ifndef WIZCHIP_HTTP_REQUEST_H
#define WIZCHIP_HTTP_REQUEST_H

#include "wizchip/url.h"
#include "wizchip/stream.h"
#include <etl/vector.h>

namespace Project::wizchip::http {
    struct Request {
        static Request parse(etl::Vector<uint8_t> buf);
        Stream dump();

        std::string method;
        URL path;
        std::string version;
        etl::UnorderedMap<std::string, std::string> headers;
        std::string body;
    };
}

#endif