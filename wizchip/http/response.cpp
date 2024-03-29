#include "wizchip/http/response.h"

using namespace Project;
using namespace Project::wizchip;

auto http::Response::parse(const uint8_t* buf, size_t len) -> Response {
    auto sv = etl::string_view(buf, len);
    auto res = Response{};

    auto methods = sv.split<3>(" ");
    if (methods.len() < 3)
        return res;
    
    res.version = methods[0];
    res.status = methods[1].to_int();
    res.status_string = methods[2].split<1>("\n")[0];
    
    sv = res.status_string.end() + 1;
    if (res.status_string and res.status_string.back() == '\r')
        res.status_string = res.status_string.substr(0, res.status_string.len() - 1);
    
    auto head_end = sv.find("\r\n\r\n");
    auto body_start = head_end + 4;
    if (head_end == sv.len()) {
        head_end = sv.find("\n\n");
        body_start = head_end + 2;
    }
    // else bad response
    
    res.head = sv.substr(0, head_end);
    res.body = sv.substr(body_start, sv.len());

    if (res.body.begin() > reinterpret_cast<const char*>(buf + len)) {
        res.body = {};
    }
    else {
        auto body_len = len - (res.body.begin() - reinterpret_cast<const char*>(buf));
        res.body = etl::string_view(res.body.begin(), body_len); 
    }
    
    return res;
} 

auto http_request_response_get_head(etl::StringView head, etl::StringView key) -> etl::StringView;

auto http::Response::get_head(etl::StringView key) const -> etl::StringView {
    return http_request_response_get_head(head, key);
}