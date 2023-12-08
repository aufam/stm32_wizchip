#ifndef WIZCHIP_HTTP_SERVER_H
#define WIZCHIP_HTTP_SERVER_H

#include "etl/function.h"
#include "wizchip/ethernet.h"
#include "wizchip/http/request.h"
#include "wizchip/http/response.h"

namespace Project::wizchip::http {

    class Server : public Socket {
    public:
        struct Args {
            Ethernet& ethernet;
            int port;
        };
        
        constexpr explicit Server(Args args) : Socket({
            .ethernet=args.ethernet, 
            .port=args.port, 
            .keep_alive=true,
        }) {}

        using HandlerFunction = etl::Function<void(const Request&, Response&), void*>;
        struct Handler {
            const char* method, * url;
            HandlerFunction function;
        };

        Server& add(Handler handler);

        Server& Get(const char* url, HandlerFunction handler) { return add({"GET", url, handler}); }
        Server& Put(const char* url, HandlerFunction handler) { return add({"PUT", url, handler}); }
        Server& Post(const char* url, HandlerFunction handler) { return add({"POST", url, handler}); }
        Server& Patch(const char* url, HandlerFunction handler) { return add({"PATCH", url, handler}); }
        
        Server& Head(const char* url, HandlerFunction handler) { return add({"HEAD", url, handler}); }
        Server& Trace(const char* url, HandlerFunction handler) { return add({"TRACE", url, handler}); }
        Server& Delete(const char* url, HandlerFunction handler) { return add({"DELETE", url, handler}); }
        Server& Options(const char* url, HandlerFunction handler) { return add({"OPTIONS", url, handler}); }

        Server& start();
        Server& stop();

        /// default: reserve 4 sockets from the ethernet
        int _number_of_socket = 4;
    
    protected:
        void process(int socket_number, const uint8_t* rxBuffer, size_t len) override;

        etl::Array<Handler, 16> handlers = {};
        int handlers_cnt = 0;
    };
}

#endif