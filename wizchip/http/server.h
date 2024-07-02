#ifndef WIZCHIP_HTTP_SERVER_H
#define WIZCHIP_HTTP_SERVER_H

#include "wizchip/tcp/server.h"
#include "wizchip/http/request.h"
#include "wizchip/http/response.h"
#include "etl/json_serialize.h"
#include "etl/json_deserialize.h"
#include <functional>

namespace Project::wizchip::http {
    class Server : public tcp::Server {
    public:
        using RouterFunction = std::function<void(const Request&, Response&)>;
        using HeaderGenerator = etl::UnorderedMap<std::string, std::function<std::string(const Request&, const Response&)>>;

        struct Router {
            std::string path;
            etl::Vector<const char*> methods;
            RouterFunction function;
        };

        struct Error {
            int status;
            std::string what;
        };

        template <typename T>
        struct RouterArg {
            const char* name;
            T get_default;
            static constexpr bool has_default = true;
            static constexpr bool is_function = false;
            static constexpr bool has_request_param = false;
            static constexpr bool is_return_type_result = false;
            using value_type = T;
        };
    
        template <typename T> struct is_router_result : etl::false_type {};
        template <typename T> struct is_router_result<etl::Result<T, Error>> : etl::true_type {};
        template <typename T> static constexpr bool is_router_result_v = is_router_result<T>::value;
        
        template <typename T> struct router_result;
        template <typename T> struct router_result<etl::Result<T, Error>> { using value_type = T; };
        template <typename T> using router_result_t = typename router_result<T>::value_type;

        template <typename... Args, typename F> 
        auto route(std::string path, etl::Vector<const char*> methods, std::tuple<RouterArg<Args>...> args, F&& handler) {
            return route_(etl::move(path), etl::move(methods), etl::move(args), std::function(etl::forward<F>(handler)));
        }
        
        template <typename... Args, typename F> 
        auto Get(std::string path, std::tuple<RouterArg<Args>...> args, F&& handler) {
            return route(etl::move(path), {"GET"}, etl::move(args), etl::forward<F>(handler));
        }

        template <typename... Args, typename F> 
        auto Post(std::string path, std::tuple<RouterArg<Args>...> args, F&& handler) {
            return route(etl::move(path), {"POST"}, etl::move(args), etl::forward<F>(handler));
        }

        template <typename... Args, typename F>  
        auto Patch(std::string path, std::tuple<RouterArg<Args>...> args, F&& handler) {
            return route(etl::move(path), {"PATCH"}, etl::move(args), etl::forward<F>(handler));
        }

        template <typename... Args, typename F>  
        auto Put(std::string path, std::tuple<RouterArg<Args>...> args, F&& handler) {
            return route(etl::move(path), {"PUT"}, etl::move(args), etl::forward<F>(handler));
        }

        template <typename... Args, typename F> 
        auto Head(std::string path, std::tuple<RouterArg<Args>...> args, F&& handler) {
            return route(etl::move(path), {"HEAD"}, etl::move(args), etl::forward<F>(handler));
        }

        template <typename... Args, typename F> 
        auto Trace(std::string path, std::tuple<RouterArg<Args>...> args, F&& handler) {
            return route(etl::move(path), {"Trace"}, etl::move(args), etl::forward<F>(handler));
        }

        template <typename... Args, typename F> 
        auto Delete(std::string path, std::tuple<RouterArg<Args>...> args, F&& handler) {
            return route(etl::move(path), {"DELETE"}, etl::move(args), etl::forward<F>(handler));
        }

        template <typename... Args, typename F> 
        auto Options(std::string path, std::tuple<RouterArg<Args>...> args, F&& handler) {
            return route(etl::move(path), {"OPTIONS"}, etl::move(args), etl::forward<F>(handler));
        }

        HeaderGenerator global_headers;
        std::function<void(const Request&, const Response&)> logger = {};
        std::function<void(Error, const Request&, Response&)> error_handler = default_error_handler;
        etl::LinkedList<Router> routers;

        etl::Time get_elapsed_time();
    
    protected:
        etl::Vector<uint8_t> response(etl::Vector<uint8_t>) override;
        etl::UnorderedMap<void*, etl::Time> responses_start_time;

        template <typename... RouterArgs, typename R, typename ...HandlerArgs>
        auto route_(
            std::string path, 
            etl::Vector<const char*> methods, 
            std::tuple<RouterArg<RouterArgs>...> args,
            std::function<R(HandlerArgs...)> handler
        ) {
            static_assert(sizeof...(RouterArgs) == sizeof...(HandlerArgs));

            RouterFunction function = [this, args=etl::move(args), handler] (const Request& req, Response& res) {
                // process each args
                std::tuple<etl::Result<HandlerArgs, Error>...> arg_values = std::apply([&](const auto&... items) {
                    return std::tuple { process_arg<HandlerArgs>(items, req)... };
                }, args);

                // check for err
                Error* err = nullptr;
                auto check_err = [&](auto& item) {
                    if (err == nullptr && item.is_err()) {
                        err = &item.unwrap_err();
                    }
                };
                std::apply([&](auto&... args) { ((check_err(args)), ...); }, arg_values);
                if (err) {
                    return error_handler(etl::move(*err), req, res);
                }
               
                // apply handler
                if constexpr (etl::is_same_v<R, void>) {
                    std::apply([&](auto&... args) { handler(etl::move(args.unwrap())...); }, arg_values);
                } else {
                    // unwrap each arg values and apply to handler
                    R result = std::apply([&](auto&... args) { return handler(etl::move(args.unwrap())...); }, arg_values);
                    if constexpr (is_router_result_v<R>) {
                        if (result.is_err()) {
                            return error_handler(etl::move(result.unwrap_err()), req, res);
                        }
                        if constexpr (not etl::is_same_v<router_result_t<R>, void>) {
                            process_result(result.unwrap(), res);
                        }
                    } else {
                        process_result(result, res);
                    }
                } 
            };

            routers.push(Router{etl::move(path), etl::move(methods), etl::move(function)});
            return handler;
        }

        static void default_error_handler(Error err, const Request&, Response& res) {
            res.status = err.status;
            res.body = etl::move(err.what);
        }

        static Error internal_error(const char* err) {
            return {StatusInternalServerError, std::string(err)};
        }
        
        template <typename T, typename Arg> static etl::Result<T, Error>
        process_arg(const RouterArg<Arg>& arg, const Request& req) {
            static_assert(etl::is_same_v<Arg, void> || etl::is_same_v<typename RouterArg<Arg>::value_type, T>);
            const std::string key = arg.name;
            if (key[0] == '$') {
                return get_request_parameter<T>(key, req);
            } else if (req.headers.has(key)) {
                return convert_std_string_to_any<T>(req.headers[key]);
            } else if (req.path.queries.has(key)) {
                return convert_std_string_to_any<T>(req.path.queries[key]);
            } else {
                if constexpr (RouterArg<Arg>::has_default) {
                    if constexpr (RouterArg<Arg>::is_function) {
                        if constexpr (RouterArg<Arg>::has_request_param) {
                            if constexpr (RouterArg<Arg>::is_return_type_result) {
                                return arg.get_default(req);
                            } else {
                                return etl::Ok(arg.get_default(req));
                            }
                        } else {
                            if constexpr (RouterArg<Arg>::is_return_type_result) {
                                return arg.get_default();
                            } else {
                                return etl::Ok(arg.get_default());
                            }
                        }
                    } else {
                        return etl::Ok(arg.get_default);
                    }
                } else {
                    return etl::Err(Error{StatusBadRequest, std::string() + "arg '" + arg.name + "' not found"});
                }
            }
        }

        template <typename T> static void
        process_result(T& result, Response& res) {
            if constexpr (etl::is_same_v<T, std::string>) {
                res.body = etl::move(result);
                res.headers["Content-Type"] = "text/plain";
            } else if constexpr (etl::is_string_v<T>) {
                res.body = std::string(result.data(), result.len());
                res.headers["Content-Type"] = "text/plain";
            } else if constexpr (etl::is_same_v<T, std::string_view> || etl::is_same_v<T, const char*>) {
                res.body = result;
                res.headers["Content-Type"] = "text/plain";
            } else if constexpr (etl::is_same_v<T, etl::StringView>) {
                res.body = std::string(result.data(), result.len());
                res.headers["Content-Type"] = "text/plain";
            } else {
                res.body = etl::json::serialize(result);
                res.headers["Content-Type"] = "application/json";
            }
        }
        
        template <typename T> static etl::Result<T, Error>
        get_request_parameter(const std::string& key, const Request& req) {
            if (key == "$request") {
                return get_request<T>(req);
            } else if (key == "$url") {
                return get_url<T>(req);
            } else if (key == "$headers") {
                return get_headers<T>(req);
            } else if (key == "$queries") {
                return get_queries<T>(req);
            } else if (key == "$path") {
                return convert_std_string_to_other_string<T>(req.path.path);
            } else if (key == "$full_path") {
                return convert_std_string_to_other_string<T>(req.path.full_path);
            } else if (key == "$fragment") {
                return convert_std_string_to_other_string<T>(req.path.fragment);
            } else if (key == "$version") {
                return convert_std_string_to_other_string<T>(req.version);
            } else if (key == "$method") {
                return convert_std_string_to_other_string<T>(req.method);
            } else if (key == "$body") {
                return convert_std_string_to_other_string<T>(req.body);
            } else {
                std::string_view content_type = [&]() -> std::string_view {
                    if (req.headers.has("Content-Type")) {
                        return req.headers["Content-Type"];
                    } else if (req.headers.has("content-type")) {
                        return req.headers["content-type"];
                    } else {
                        return "";
                    }
                }();

                if (key == "$json" && content_type == "application/json") {
                    return get_json<T>(req.body);
                } else if (key == "$text" && content_type == "text/plain") {
                    return convert_std_string_to_other_string<T>(req.body);
                } else {
                    return etl::Err(internal_error("unknown arg"));
                }
            } 
        }

        template <typename T> static etl::Result<T, Error>
        get_request(const Request& req) {
            if constexpr (etl::is_same_v<T, etl::Ref<const Request>>) {
                return etl::Ok(etl::ref_const(req));
            } else {
                return etl::Err(internal_error("arg type $request must be etl::Ref<const Request>"));
            }
        }

        template <typename T> static etl::Result<T, Error>
        get_url(const Request& req) {
            if constexpr (etl::is_same_v<T, etl::Ref<const URL>>) {
                return etl::Ok(etl::ref_const(req.path));
            } else {
                return etl::Err(internal_error("arg type $url must be etl::Ref<const URL>"));
            }
        }

        template <typename T> static etl::Result<T, Error>
        get_headers(const Request& req) {
            if constexpr (etl::is_same_v<T, etl::Ref<const decltype(Request::headers)>>) {
                return etl::Ok(etl::ref_const(req.headers));
            } else {
                return etl::Err(internal_error("arg type $headers must be etl::Ref<const decltype(Request::headers)>"));
            }
        }

        template <typename T> static etl::Result<T, Error>
        get_queries(const Request& req) {
            if constexpr (etl::is_same_v<T, etl::Ref<const decltype(URL::queries)>>) {
                return etl::Ok(etl::ref_const(req.path.queries));
            } else {
                return etl::Err(internal_error("arg type $queries must be etl::Ref<const decltype(URL::queries)>"));
            }
        }

        template <typename T> static etl::Result<T, Error>
        get_json(const std::string& str) {
            return etl::json::deserialize<T>(str).except(internal_error);
        }

        template <typename T> static etl::Result<T, Error> 
        convert_std_string_to_other_string(const std::string& str) {
            if constexpr (etl::is_same_v<T, std::string> || etl::is_same_v<T, std::string_view> || etl::is_same_v<T, etl::StringView>) {
                return etl::Ok(T(str.data(), str.size()));
            } else if constexpr (etl::is_string_v<T>) {
                return etl::Ok(T("%.*s", str.size(), str.data()));
            } else if constexpr (etl::is_same_v<T, const char*>) {
                return etl::Ok(str.c_str());
            } else {
                return etl::Err(internal_error("cannot convert to string"));
            }
        }

        template <typename T> static etl::Result<T, Error> 
        convert_std_string_to_any(const std::string& str) {
            etl::Result<T, Error> res = convert_std_string_to_other_string<T>(str);
            if (res.is_ok()) {
                return res;
            } else {
                return get_json<T>(str);
            }
        }
    };

    template <>
    struct Server::RouterArg<void> {
        const char* name;
        static constexpr bool is_dependency = false;
        static constexpr bool has_default = false;
        static constexpr bool is_function = false;
        static constexpr bool has_request_param = false;
        static constexpr bool is_return_type_result = false;
        using value_type = void;
    };

    template <typename T>
    struct Server::RouterArg<std::function<T()>> {
        const char* name;
        std::function<T()> get_default;
        static constexpr bool has_default = true;
        static constexpr bool is_function = true;
        static constexpr bool has_request_param = false;
        static constexpr bool is_return_type_result = false;
        using value_type = T;
    };

    template <typename T>
    struct Server::RouterArg<std::function<etl::Result<T, Server::Error>()>> {
        const char* name;
        std::function<etl::Result<T, Error>()> get_default;
        static constexpr bool has_default = true;
        static constexpr bool is_function = true;
        static constexpr bool has_request_param = false;
        static constexpr bool is_return_type_result = true;
        using value_type = T;
    };

    template <typename T>
    struct Server::RouterArg<std::function<T(const Request&)>> {
        const char* name;
        std::function<T(const Request&)> get_default;
        static constexpr bool has_default = true;
        static constexpr bool is_function = true;
        static constexpr bool has_request_param = true;
        static constexpr bool is_return_type_result = false;
        using value_type = T;
    };

    template <typename T>
    struct Server::RouterArg<std::function<etl::Result<T, Server::Error>(const Request&)>> {
        const char* name;
        std::function<etl::Result<T, Error>(const Request&)> get_default;
        static constexpr bool has_default = true;
        static constexpr bool is_function = true;
        static constexpr bool has_request_param = true;
        static constexpr bool is_return_type_result = true;
        using value_type = T;
    };
}

namespace Project::wizchip::http::arg {
    template <typename... Args>
    using args = std::tuple<Args...>;

    inline auto arg(const char* name) {
        return Server::RouterArg<void> {name};
    }

    template <typename F>
    auto depends(F&& depends_function) {
        std::function f = etl::forward<F>(depends_function);
        return Server::RouterArg<decltype(f)> { "", etl::move(f) };
    }

    template <typename T>
    auto default_val(const char* name, T&& default_value) {
        return Server::RouterArg<etl::decay_t<T>> { name, etl::forward<T>(default_value) };
    }

    template <typename F>
    auto default_fn(const char* name, F&& default_function) {
        std::function f = etl::forward<F>(default_function);
        return Server::RouterArg<decltype(f)> { name, etl::move(f) };
    }

    inline static constexpr Server::RouterArg<void> request { "$request" };
    inline static constexpr Server::RouterArg<void> url { "$url" };
    inline static constexpr Server::RouterArg<void> headers { "$headers" };
    inline static constexpr Server::RouterArg<void> queries { "$queries" };
    inline static constexpr Server::RouterArg<void> path { "$path" };
    inline static constexpr Server::RouterArg<void> full_path { "$full_path" };
    inline static constexpr Server::RouterArg<void> fragment { "$fragment" };
    inline static constexpr Server::RouterArg<void> version { "$version" };
    inline static constexpr Server::RouterArg<void> method { "$method" };
    inline static constexpr Server::RouterArg<void> body { "$body" };
    inline static constexpr Server::RouterArg<void> json { "$json" };
    inline static constexpr Server::RouterArg<void> text { "$text" };
}

#endif
