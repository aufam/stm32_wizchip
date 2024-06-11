#ifndef WIZCHIP_HTTP_SERVER_H
#define WIZCHIP_HTTP_SERVER_H

#include "wizchip/tcp/server.h"
#include "wizchip/http/request.h"
#include "wizchip/http/response.h"
#include <functional>

namespace Project::wizchip::http {
    class Server : public tcp::Server {
    public:
        struct Args {
            int port;
        };

        Server(Args args) : tcp::Server(tcp::Server::Args{
            .port=args.port, 
            .response_function=etl::bind<&Server::_response_function>(this)
        }) {}

        using RouterFunction = std::function<void(const Request&, Response&)>;
        using HeaderGenerator = etl::UnorderedMap<std::string, std::function<std::string()>>;

        struct Router {
            etl::Vector<const char*> methods;
            std::string path;
            const char* content_type = "";
            RouterFunction function;
        };

        Server& route(Router router) {
            routers.push(etl::move(router));
            return *this;
        }

        Server& Get(std::string path, const char* content_type, RouterFunction function) { 
            return route(Router{{"GET"}, etl::move(path), content_type, etl::move(function)}); 
        }
        Server& Put(std::string path, const char* content_type, RouterFunction function) { 
            return route(Router{{"PUT"}, etl::move(path), content_type, etl::move(function)}); 
        }
        Server& Post(std::string path, const char* content_type, RouterFunction function) { 
            return route(Router{{"POST"}, etl::move(path), content_type, etl::move(function)}); 
        }
        Server& Patch(std::string path, const char* content_type, RouterFunction function) { 
            return route(Router{{"PATCH"}, etl::move(path), content_type, etl::move(function)}); 
        }
        Server& Head(std::string path, const char* content_type, RouterFunction function) { 
            return route(Router{{"HEAD"}, etl::move(path), content_type, etl::move(function)}); 
        }
        Server& Trace(std::string path, const char* content_type, RouterFunction function) { 
            return route(Router{{"TRACE"}, etl::move(path), content_type, etl::move(function)}); 
        }
        Server& Delete(std::string path, const char* content_type, RouterFunction function) { 
            return route(Router{{"DELETE"}, etl::move(path), content_type, etl::move(function)}); 
        }
        Server& Options(std::string path, const char* content_type, RouterFunction function) { 
            return route(Router{{"OPTIONS"}, etl::move(path), content_type, etl::move(function)}); 
        }

        HeaderGenerator global_headers;
        std::function<void(const Request&, const Response&)> logger = {};
        [[read_only]] etl::LinkedList<Router> routers;
    
    protected:
        etl::Vector<uint8_t> _response_function(etl::Vector<uint8_t>);
    };
}

#endif