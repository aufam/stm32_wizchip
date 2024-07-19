#include "request.h"
#include "etl/this_thread.h"
#include "etl/keywords.h"

using namespace wizchip;

void wizchip_http_request_response_parse_headers_body(
    etl::StringView sv, 
    etl::UnorderedMap<std::string, std::string>& headers, 
    std::string& body
);

Stream::InRule wizchip_http_request_response_string_to_stream_rule(
    std::string str
);

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

auto http::Request::dump() -> Stream {
    static const uint8_t space[] = {' '};
    static const uint8_t cr_lf[] = {'\r', '\n'};
    static const uint8_t colon[] = {':', ' '};

    Stream s;
    s << wizchip_http_request_response_string_to_stream_rule(mv | method);
    s << etl::iter(space);
    s << wizchip_http_request_response_string_to_stream_rule(mv | path.url);
    s << etl::iter(space);
    s << wizchip_http_request_response_string_to_stream_rule(mv | version);
    s << etl::iter(cr_lf);
    
    for (auto &[key, value] : headers) {
        s << wizchip_http_request_response_string_to_stream_rule(mv | key);
        s << etl::iter(colon);
        s << wizchip_http_request_response_string_to_stream_rule(mv | value);
        s << etl::iter(cr_lf);
    }

    s << etl::iter(cr_lf);
    if (!body.empty()) {
        s << wizchip_http_request_response_string_to_stream_rule(mv | body);
    }

    return s;
}


void wizchip_http_request_response_parse_headers_body(
    etl::StringView sv, 
    etl::UnorderedMap<std::string, std::string>& headers, 
    std::string& body
) {
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
    if (body_length > 0) {
        body.assign(sv.data() + body_start, body_length);
    }

    for (auto line : hsv.split<16>("\n")) {
        auto kv = line.split<2>(":");
        auto key = kv[0];
        auto value = kv[1];

        if (value and value.back() == '\r')
            value = value.substr(0, value.len() - 1);
        
        while (value and value.front() == ' ') 
            value = value.substr(1, value.len() - 1);
        
        headers[std::string(key.data(), key.end())] = std::string(value.data(), value.end()); 
    }
}

Stream::InRule wizchip_http_request_response_string_to_stream_rule(
    std::string str
) {
    return [str=etl::move(str)]() {
        auto ptr = reinterpret_cast<const uint8_t*>(str.data());
        return etl::iter(ptr, ptr + str.size());
    };
}