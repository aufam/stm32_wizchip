#ifndef WIZCHIP_HTTP_CLIENT_H
#define WIZCHIP_HTTP_CLIENT_H

#include "wizchip/tcp/client.h"
#include "wizchip/http/request.h"
#include "wizchip/http/response.h"

namespace Project::wizchip::http {

    class Client : public tcp::Client {
    public:
        using tcp::Client::Client;

        explicit Client(std::string host);

        using HandlerFunction = etl::Function<void(const Response&), void*>;

        etl::Future<Response> request(Request req);

        etl::Future<Response> Get(std::string path, etl::UnorderedMap<std::string, std::string> headers = {}, std::string body = "") { 
            return request({.method="GET", .path=etl::move(path), .version="HTTP/1.1", .headers=etl::move(headers), .body=etl::move(body)}); 
        }
        etl::Future<Response> Put(std::string path, etl::UnorderedMap<std::string, std::string> headers, std::string body) {
            return request({.method="PUT", .path=etl::move(path), .version="HTTP/1.1", .headers=etl::move(headers), .body=etl::move(body)}); 
        }
        etl::Future<Response> Post(std::string path, etl::UnorderedMap<std::string, std::string> headers, std::string body) {
            return request({.method="POST", .path=etl::move(path), .version="HTTP/1.1", .headers=etl::move(headers), .body=etl::move(body)}); 
        }
        etl::Future<Response> Patch(std::string path, etl::UnorderedMap<std::string, std::string> headers, std::string body) {
            return request({.method="PATCH", .path=etl::move(path), .version="HTTP/1.1", .headers=etl::move(headers), .body=etl::move(body)}); 
        }
        etl::Future<Response> Head(std::string path, etl::UnorderedMap<std::string, std::string> headers = {}, std::string body = "") {
            return request({.method="HEAD", .path=etl::move(path), .version="HTTP/1.1", .headers=etl::move(headers), .body=etl::move(body)}); 
        }
        etl::Future<Response> Trace(std::string path, etl::UnorderedMap<std::string, std::string> headers, std::string body) {
            return request({.method="TRACE", .path=etl::move(path), .version="HTTP/1.1", .headers=etl::move(headers), .body=etl::move(body)}); 
        }
        etl::Future<Response> Delete(std::string path, etl::UnorderedMap<std::string, std::string> headers = {}, std::string body = "") {
            return request({.method="DELETE", .path=etl::move(path), .version="HTTP/1.1", .headers=etl::move(headers), .body=etl::move(body)}); 
        }
        etl::Future<Response> Options(std::string path, etl::UnorderedMap<std::string, std::string> headers = {}, std::string body = "") {
            return request({.method="OPTIONS", .path=etl::move(path), .version="HTTP/1.1", .headers=etl::move(headers), .body=etl::move(body)}); 
        }

    // private:
    //     std::string host_port;
    };

    struct HeadersBody {
        etl::UnorderedMap<std::string, std::string> headers = {};
        std::string body = "";
    };

    etl::Future<Response> request(std::string method, URL url, HeadersBody headers_body);

    inline etl::Future<Response> Get(URL url, HeadersBody headers_body = {}) { 
        return request("GET", url, etl::move(headers_body)); 
    }
    inline etl::Future<Response> Put(URL url, HeadersBody headers_body) { 
        return request("PUT", url, etl::move(headers_body)); 
    }
    inline etl::Future<Response> Post(URL url, HeadersBody headers_body) { 
        return request("POST", url, etl::move(headers_body)); 
    }
    inline etl::Future<Response> Patch(URL url, HeadersBody headers_body) { 
        return request("PATCH", url, etl::move(headers_body)); 
    }
    inline etl::Future<Response> Head(URL url, HeadersBody headers_body = {}) { 
        return request("HEAD", url, etl::move(headers_body)); 
    }
    inline etl::Future<Response> Trace(URL url, HeadersBody headers_body) { 
        return request("TRACE", url, etl::move(headers_body)); 
    }
    inline etl::Future<Response> Delete(URL url, HeadersBody headers_body = {}) { 
        return request("DELETE", url, etl::move(headers_body)); 
    }
    inline etl::Future<Response> Options(URL url, HeadersBody headers_body = {}) { 
        return request("OPTIONS", url, etl::move(headers_body)); 
    }
}

#endif