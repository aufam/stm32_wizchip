#ifndef WIZCHIP_HTTP_RESPONSE_H
#define WIZCHIP_HTTP_RESPONSE_H

#include "etl/string_view.h"
#include "etl/array.h"

namespace Project::wizchip::http {
    struct Response {
        static Response parse(const uint8_t* buf, size_t len);

        etl::StringView version;
        int status = 200;
        etl::StringView status_string = "OK";
        etl::StringView head;
        etl::StringView body;

        etl::StringView get_head(etl::StringView key) const;
    };
}

#endif