#include "response.h"
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
    
    Response res;
    res.version = std::string(version.data(), version.len());
    res.status = status;
    res.status_string = std::string(status_string.data(), status_string.len());
    wizchip_http_request_response_parse_headers_body(sv, res.headers, res.body);
    
    return res;
}

auto http::Response::dump() -> Stream {
    static const uint8_t space[] = {' '};
    static const uint8_t cr_lf[] = {'\r', '\n'};
    static const uint8_t colon[] = {':', ' '};

    Stream s;
    s << wizchip_http_request_response_string_to_stream_rule(mv | version);
    s << etl::iter(space);
    s << wizchip_http_request_response_string_to_stream_rule(std::to_string(status));
    s << etl::iter(space);
    s << wizchip_http_request_response_string_to_stream_rule(mv | status_string);
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
