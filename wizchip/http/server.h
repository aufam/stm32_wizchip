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

        void add(Handler handler);

        void Get(const char* url, HandlerFunction handler) { add({"GET", url, handler}); }
        void Put(const char* url, HandlerFunction handler) { add({"PUT", url, handler}); }
        void Post(const char* url, HandlerFunction handler) { add({"POST", url, handler}); }
        void Patch(const char* url, HandlerFunction handler) { add({"PATCH", url, handler}); }
        
        void Head(const char* url, HandlerFunction handler) { add({"HEAD", url, handler}); }
        void Trace(const char* url, HandlerFunction handler) { add({"TRACE", url, handler}); }
        void Delete(const char* url, HandlerFunction handler) { add({"DELETE", url, handler}); }
        void Options(const char* url, HandlerFunction handler) { add({"OPTIONS", url, handler}); }

        void listen();
        void stop();
    
    protected:
        void process(int socket_number, const uint8_t* rxBuffer, size_t len) override;

        static uint8_t txData[WIZCHIP_BUFFER_LENGTH];
        etl::Array<Handler, 16> handlers = {};
        int handlers_cnt = 0;
    };
}

#endif