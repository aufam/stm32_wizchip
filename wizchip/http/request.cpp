#include "request.h"
#include "etl/heap.h"
#include "etl/thread.h"

using namespace Project;
using namespace Project::wizchip;
using namespace Project::etl::literals;

auto wizchip_http_request_response_parse_headers_body(etl::StringView sv, etl::UnorderedMap<std::string, std::string>& headers, std::string& body) -> void;

auto http::Request::parse(etl::Vector<uint8_t> buf) -> Request {
    auto sv = etl::string_view(buf.data(), buf.len());

    auto request_line = sv.split<3>(" ");
    if (request_line.len() < 3)
        return {};
    
    auto method = request_line[0];
    auto path = request_line[1];
    auto version = request_line[2].split<1>("\n")[0];
    
    sv = version.end() + 1;
    if (version and version.back() == '\r')
        version = version.substr(0, version.len() - 1);
    
    Request req;
    req.method = std::string(method.data(), method.len());
    req.path = URL(path);
    req.version = std::string(version.data(), version.len());
    wizchip_http_request_response_parse_headers_body(sv, req.headers, req.body);

    if (req.headers.has("Host")) {
        req.path.host = req.headers["Host"];
    } else if (req.headers.has("host")) {
        req.path.host = req.headers["host"];
    } 
    
    return req;
} 

auto http::Request::dump() const -> etl::Vector<uint8_t> {
    auto total = method.size() + 1 + path.url.size() + 1 + version.size() + 2;
    for (auto &[key, value] : headers) {
        total += key.size() + 2 + value.size() + 2;
    }
    total += 2 + body.size();

    // TODO: some how the body length can't be 0
    total += etl::max(body.size(), 1u);

    while (total >= etl::heap::freeSize) {
        etl::this_thread::sleep(1ms);
    }

    auto res = etl::vector_allocate<uint8_t>(total);
    auto ptr = reinterpret_cast<char*>(res.data());
    ptr[total - 1] = '\0';

    auto n = ::snprintf(ptr, total, "%s %s %s\r\n", method.c_str(), path.url.c_str(), version.c_str());
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

auto wizchip_http_request_response_parse_headers_body(etl::StringView sv, etl::UnorderedMap<std::string, std::string>& headers, std::string& body) -> void {
    auto head_end = sv.find("\r\n\r\n");
    auto body_start = head_end + 4;
    if (head_end >= sv.len()) {
        head_end = sv.find("\n\n");
        body_start -= 2;
        if (head_end >= sv.len()) {
            body_start -= 2;
        }
    }

    auto hsv = sv.substr(0, head_end);
    auto body_length = sv.len() - body_start;

    for (auto line : hsv.split<16>("\n")) {
        auto kv = line.split<2>(":");
        auto key = kv[0];
        auto value = kv[1];

        if (value and value.back() == '\r')
            value = value.substr(0, value.len() - 1);
        
        while (value and value.front() == ' ') 
            value = value.substr(1, value.len() - 1);
        
        if (key == "Content-Length" or key == "content-length")
            body_length = value.to_int_or(body_length);
        
        headers[std::string(key.data(), key.end())] = std::string(value.data(), value.end()); 
    }

    body.assign(sv.data() + body_start, body_length);
}
