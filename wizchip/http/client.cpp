#include "Ethernet/socket.h"
#include "wizchip/http/client.h"
#include "etl/heap.h"
#include "etl/keywords.h"

using namespace Project::wizchip;

http::Client::Client(std::string host) : Client(tcp::Client::Args{{}, 0}) {
    auto [h, p] = detail::ipv4_port_to_pair(host.c_str());
    this->host = etl::move(h);
    this->port = p;
}
void debug_cnt(const char* format, size_t s1, size_t s2);
void debug_str(const char* format, const char* str, size_t len);

auto http::Client::request(http::Request req) -> etl::Future<http::Response> {
    req.headers["User-Agent"] = "stm32_wizchip/" WIZCHIP_VERSION;
    if (!req.body.empty()) req.headers["Content-Length"] = std::to_string(req.body.size());
    req.headers["Host"] = std::to_string(host[0]) + '.' + 
                          std::to_string(host[1]) + '.' + 
                          std::to_string(host[2]) + '.' + 
                          std::to_string(host[3]) + ':' + 
                          std::to_string(port);

    return tcp::Client::request(req.dump())
        .and_then([this](etl::Vector<uint8_t> data) -> etl::Result<http::Response, osStatus_t> {
            auto response = http::Response::parse(mv | data);

            int len = 0;
            if (response.headers.has("Content-Length")) {
                len = etl::string_view(response.headers["Content-Length"].c_str()).to_int() - response.body.size();
            } else if (response.headers.has("content-length")) {
                len = etl::string_view(response.headers["content-length"].c_str()).to_int() - response.body.size();
            }

            if (int(etl::heap::freeSize) < len) {
                return etl::Err(osErrorNoMemory);
            } else if (len > 0) {
                auto body_size = response.body.size();
                response.body.resize(body_size + len);
                detail::tcp_receive_to(socket_number, reinterpret_cast<uint8_t*>(&response.body[body_size]), len);
            } else if (len < 0) {
                response.body.resize(response.body.size() + len);
            } 

            return etl::Ok(mv | response);
        });
}

auto http::request(std::string method, URL url, HeadersBody headers_body) -> etl::Future<Response> {
    return [method=etl::move(method), url=etl::move(url), headers_body=etl::move(headers_body)](etl::Time timeout) mutable -> etl::Result<Response, osStatus_t> {
        auto cli = Client(url.host);
        return cli.request(Request{
            .method=etl::move(method),
            .path=etl::move(url.full_path),
            .version="HTTP/1.1",
            .headers=etl::move(headers_body.headers),
            .body=etl::move(headers_body.body),
        }).wait(timeout);
    };
}

