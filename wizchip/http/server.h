#ifndef WIZCHIP_HTTP_SERVER_H
#define WIZCHIP_HTTP_SERVER_H

#include "etl/function.h"
#include "wizchip/ethernet.h"
#include "wizchip/http/request.h"
#include "wizchip/http/response.h"

namespace Project::wizchip::http {

    class Server : public Socket {
    public:
        constexpr explicit Server(Args args) : Socket(args) {}

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
        bool is_running() const { return _is_running; }

        etl::Function<void(const Request&, const Response&), void*> debug;

        /// default: reserve 4 sockets from the ethernet
        int _number_of_socket = 4;
    
    protected:
        int on_init(int socket_number) override;
        int on_listen(int socket_number) override;
        int on_established(int socket_number) override;
        int on_close_wait(int socket_number) override;
        int on_closed(int socket_number) override;
        void process(int socket_number, const uint8_t* rxBuffer, size_t len);

        etl::Array<Handler, 16> handlers = {};
        int handlers_cnt = 0;
        bool _is_running = false;
    };
}

#endif