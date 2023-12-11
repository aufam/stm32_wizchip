#ifndef WIZCHIP_HTTP_REQUEST_H
#define WIZCHIP_HTTP_REQUEST_H

#include "etl/string_view.h"

namespace Project::wizchip::http {
    struct Request {
        static Request parse(const uint8_t* buf, size_t len);

        etl::StringView method, url, version, head, body;

        etl::StringView get_head(etl::StringView key) const;
        etl::StringView matches(int index) const;
    
        etl::StringMatch<16>* _matches = nullptr;
    };
}

#endif