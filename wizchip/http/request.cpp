#include "wizchip/http/request.h"

using namespace Project;
using namespace Project::wizchip;

auto http::Request::get_head(etl::StringView key) const -> etl::StringView {
    auto pos = head.find(key);
    if (pos < head.len())
        return head.substr(pos + 2, head.len() - (pos + 2)); // assuming there is ": "

    return nullptr;
}

auto http::Request::parse(const uint8_t* buf, size_t len) -> Request {
    auto sv = etl::string_view(buf, len);
    auto req = Request{};

    auto methods = sv.split<3>(" ");
    req.method = methods[0];
    req.url = methods[1];

    auto version_and_head = methods[2].split<2>("\n");
    req.version = version_and_head[0];
    if (req.version.back() == '\r')
        req.version = req.version.substr(0, req.version.len() - 1); // remove '\r'

    auto head_begin = version_and_head[1].begin();
    auto head_len = etl::string_view(head_begin).find("\r\n\r\n");
    if (head_begin[head_len] == '\0')
        head_len = etl::string_view(head_begin).find("\n\n"); // try "\n\n"

    req.head = etl::string_view(head_begin, head_len);
    req.body = etl::string_view(req.head.end() + 4);
    
    return req;
} 