#ifndef WIZCHIP_HTTP_CLIENT_H
#define WIZCHIP_HTTP_CLIENT_H

#include "etl/function.h"
#include "etl/event.h"
#include "wizchip/ethernet.h"
#include "wizchip/http/request.h"
#include "wizchip/http/response.h"

namespace Project::wizchip::http {

    class Client : public Socket {
    public:
        struct Args {
            Ethernet& ethernet;
            etl::StringView host;
            int port;
        };

        constexpr explicit Client(Args args) : Socket({args.ethernet, args.port}), host(args.host) {}

        using HandlerFunction = etl::Function<void(const Response&), void*>;

        Response request(const Request& req);
        void request(const Request& req, HandlerFunction handler);

        Response Get(etl::StringView url) { 
            return request({.method="GET", .url=url, .version="HTTP/1.1", .head="", .body=""}); 
        }

        Response Put(etl::StringView url, etl::StringView head, etl::StringView body) {
            return request({.method="PUT", .url=url, .version="HTTP/1.1", .head=head, .body=body}); 
        }

        Response Post(etl::StringView url, etl::StringView head, etl::StringView body) {
            return request({.method="POST", .url=url, .version="HTTP/1.1", .head=head, .body=body}); 
        }

        Response Patch(etl::StringView url, etl::StringView head, etl::StringView body) {
            return request({.method="PATCH", .url=url, .version="HTTP/1.1", .head=head, .body=body}); 
        }
        
        Response Head(etl::StringView url) {
            return request({.method="HEAD", .url=url, .version="HTTP/1.1", .head="", .body=""}); 
        }

        Response Trace(etl::StringView url, etl::StringView head, etl::StringView body) {
            return request({.method="TRACE", .url=url, .version="HTTP/1.1", .head=head, .body=body}); 
        }

        Response Delete(etl::StringView url, etl::StringView head = "", etl::StringView body = "") {
            return request({.method="DELETE", .url=url, .version="HTTP/1.1", .head=head, .body=body}); 
        }

        Response Options(etl::StringView url) {
            return request({.method="OPTIONS", .url=url, .version="HTTP/1.1", .head="", .body=""}); 
        }

        void Get(etl::StringView url, HandlerFunction handler) { 
            request({.method="GET", .url=url, .version="HTTP/1.1", .head="", .body=""}, handler); 
        }

        void Put(etl::StringView url, etl::StringView head, etl::StringView body, HandlerFunction handler) {
            request({.method="PUT", .url=url, .version="HTTP/1.1", .head=head, .body=body}, handler); 
        }

        void Post(etl::StringView url, etl::StringView head, etl::StringView body, HandlerFunction handler) {
            request({.method="POST", .url=url, .version="HTTP/1.1", .head=head, .body=body}, handler); 
        }

        void Patch(etl::StringView url, etl::StringView head, etl::StringView body, HandlerFunction handler) {
            request({.method="PATCH", .url=url, .version="HTTP/1.1", .head=head, .body=body}, handler); 
        }
        
        void Head(etl::StringView url, HandlerFunction handler) {
            request({.method="HEAD", .url=url, .version="HTTP/1.1", .head="", .body=""}, handler); 
        }

        void Trace(etl::StringView url, etl::StringView head, etl::StringView body, HandlerFunction handler) {
            request({.method="TRACE", .url=url, .version="HTTP/1.1", .head=head, .body=body}, handler); 
        }

        void Delete(etl::StringView url, etl::StringView head, etl::StringView body, HandlerFunction handler) {
            request({.method="DELETE", .url=url, .version="HTTP/1.1", .head=head, .body=body}, handler); 
        }

        void Options(etl::StringView url, HandlerFunction handler) {
            request({.method="OPTIONS", .url=url, .version="HTTP/1.1", .head="", .body=""}, handler); 
        }

        etl::Function<void(const Request&, const Response&), void*> debug;

        /// default timeout = 1s;
        etl::Time _timeout = etl::time::seconds(1);
    
    protected:
        int on_init(int socket_number) override;
        int on_listen(int socket_number) override;
        int on_established(int socket_number) override;
        int on_close_wait(int socket_number) override;
        int on_closed(int socket_number) override;
        void process(int socket_number, uint8_t* txBuffer);

        etl::StringView host;
        HandlerFunction handler;
        etl::Event event;
        Request _request = {};
        Response _response = {};

        enum {
            STATE_START,
            STATE_ESTABLISHED,
            STATE_STOP,
        };

        int state = STATE_START;
    };
}

#endif