#include "Ethernet/socket.h"
#include "wizchip/http/client.h"

using namespace Project::wizchip;
using namespace Project::etl::literals;

http::Client::Client(std::string host) : Client(tcp::Client::Args{{}, 0}) {
    auto [h, p] = detail::ipv4_port_to_pair(host.c_str());
    this->host = etl::move(h);
    this->port = p;
}

auto http::Client::request(http::Request req) -> etl::Future<http::Response> {
    req.headers["User-Agent"] = "stm32_wizchip/" WIZCHIP_VERSION;

    return tcp::Client::request(req.dump()).then([](etl::Vector<uint8_t> data) {
        return http::Response::parse(etl::move(data));
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

