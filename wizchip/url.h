#ifndef WIZCHIP_HTTP_URL_H
#define WIZCHIP_HTTP_URL_H

#include "etl/unordered_map.h"
#include "etl/string_view.h"
#include <string>

namespace Project::wizchip {
    struct URL {
        URL() {}
        URL(std::string url);
        URL(etl::StringView url);
        URL(const char* url) : URL(etl::StringView(url)) {}

        std::string url;
        std::string host;
        std::string path;
        std::string full_path;
        etl::UnorderedMap<std::string, std::string> queries;
        std::string fragment;
    };
}

namespace Project::wizchip::literals {
    URL operator ""_url(const char* text, size_t n);
}

#endif