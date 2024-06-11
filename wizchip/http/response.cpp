#include "response.h"
#include "etl/heap.h"
#include "etl/thread.h"

using namespace Project;
using namespace Project::wizchip;
using namespace Project::etl::literals;

auto wizchip_http_request_response_parse_headers_body(etl::StringView sv) -> etl::Pair<etl::UnorderedMap<std::string, std::string>, std::string>;

auto http::Response::parse(etl::Vector<uint8_t> buf) -> Response {
    auto sv = etl::string_view(buf.data(), buf.len());
    auto methods = sv.split<3>(" ");

    if (methods.len() < 3)
        return {};
    
    auto version = methods[0];
    auto status = methods[1].to_int();
    auto status_string = methods[2].split<1>("\n")[0];
    
    sv = status_string.end() + 1;
    if (status_string and status_string.back() == '\r')
        status_string = status_string.substr(0, status_string.len() - 1);

    auto [headers, body] = wizchip_http_request_response_parse_headers_body(sv);
    return {
        .version=std::string(version.data(), version.len()),
        .status=status,
        .status_string=std::string(status_string.data(), status_string.len()),
        .headers=etl::move(headers),
        .body=etl::move(body),
    };
} 

auto http::Response::dump() const -> etl::Vector<uint8_t> {
    auto status_num = std::to_string(status);

    auto total = version.size() + 1 + status_num.size() + 1 + status_string.size() + 2;
    for (auto &[key, value] : headers) {
        total += key.size() + 2 + value.size() + 2;
    }
    total += 2 + body.size();


    while (total >= etl::heap::freeSize) {
        etl::this_thread::sleep(1ms);
    }

    auto res = etl::vector_allocate<uint8_t>(total);
    auto ptr = reinterpret_cast<char*>(res.data());

    auto n = ::snprintf(ptr, total, "%s %s %s\r\n", version.c_str(), status_num.c_str(), status_string.c_str());
    if (n < 0) return {};
    
    ptr += n;
    total -= n;

    for (auto &[key, value] : headers) {
        n = ::snprintf(ptr, total, "%s: %s\r\n", key.c_str(), value.c_str());
        if (n < 0) return {};
        
        ptr += n;
        total -= n;
    }

    n = ::snprintf(ptr, total, "\r\n");
    if (n < 0) return {};
    
    ptr += n;
    total -= n;

    ::memcpy(ptr, body.data(), body.size());
    return res;
}

void http::Response::set_content(std::string content, std::string content_type) {
    body = etl::move(content);
    headers["Content-Type"] = etl::move(content_type);
}