#ifndef WIZCHIP_HTTP_RESPONSE_H
#define WIZCHIP_HTTP_RESPONSE_H

#include "etl/string_view.h"
#include "etl/array.h"

namespace Project::wizchip::http {
    struct Response {
        struct HeadKeyValue { etl::StringView key, value; };

        etl::StringView version;
        int status = -1;
        etl::StringView status_string;
        etl::Array<HeadKeyValue, 16> head;
        etl::StringView body;

        void set_head(etl::StringView key, etl::StringView value);
        etl::StringView get_head(etl::StringView key) const;

        etl::StringView matches(int index) const {
            return matches_ ? (*matches_)[index] : nullptr;
        }
    
    private:
        friend class Server;
        etl::StringMatch<16>* matches_ = nullptr;
    };
}

#endif