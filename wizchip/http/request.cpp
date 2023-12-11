#include "wizchip/http/request.h"

using namespace Project;
using namespace Project::wizchip;

auto http::Request::parse(const uint8_t* buf, size_t len) -> Request {
    auto sv = etl::string_view(buf, len);
    auto req = Request{};

    auto methods = sv.split<3>(" ");
    req.method = methods[0];
    req.url = methods[1];
    req.version = methods[2].split<1>("\n")[0];
    
    sv = req.version.end() + 1;
    if (req.version and req.version.back() == '\r')
        req.version = req.version.substr(0, req.version.len() - 1);
    
    auto head_end = sv.find("\r\n\r\n");
    auto body_start = head_end + 4;
    if (head_end == sv.len()) {
        head_end = sv.find("\n\n");
        body_start = head_end + 2;
    }
    // else bad request
    
    req.head = sv.substr(0, head_end);
    req.body = sv.substr(body_start, sv.len());

    auto body_len = len - (req.body.begin() - reinterpret_cast<const char*>(buf));
    req.body = etl::string_view(req.body.begin(), body_len); 
    
    return req;
} 

auto http_request_response_get_head(etl::StringView head, etl::StringView key) -> etl::StringView {
    auto pos = head.find(key);
    if (pos >= head.len())
        return nullptr;
    
    auto pos_end = pos + key.len();
    auto res = head.substr(pos_end, head.len() - pos_end);
    if (res.find(": ") == 0)
        res = res.substr(2, res.len() - 2);
    else if (res.find(":") == 0)
        res = res.substr(1, res.len() - 1);
    // else bad request header
    
    res = res.substr(0, res.find("\n"));
    if (res and res.back() == '\r')
        res = res.substr(0, res.len() - 1);

    return res; 
}

auto http::Request::get_head(etl::StringView key) const -> etl::StringView {
    return http_request_response_get_head(head, key);
}

auto http::Request::matches(int index) const -> etl::StringView {
    return _matches ? (*_matches)[index] : nullptr;
}