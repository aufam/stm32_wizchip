#include "Ethernet/socket.h"
#include "wizchip/http/server.h"
#include "etl/string.h"
#include <string>

using namespace Project;
using namespace Project::wizchip;

static auto status_to_string(int status) -> std::string;

auto http::Server::_response_function(etl::Vector<uint8_t> data) -> etl::Vector<uint8_t> {
    auto request = Request::parse(etl::move(data));
    auto response = Response {};

    response.version = "HTTP/1.1";
    for (auto &[header, fn] : global_headers) {
        response.headers[header] = fn();
    }

    // handling
    bool handled = false;
    for (auto &router : routers) {
        auto it = etl::find(router.methods, request.method);
        if (it != router.methods.end() and router.path == request.path.path) {
            response.status = 200;
            response.headers["Content-Type"] = router.content_type;

            router.function(request, response);
            handled = true;
            break;
        }
    }

    if (not handled) response.status = 404;
    if (response.status_string.empty()) response.status_string = status_to_string(response.status);
    if (logger) logger(request, response);

    return response.dump();
}

static auto status_to_string(int status) -> std::string {
    switch (status) {
        // 100
        case http::StatusContinue           : return "Continue"; // RFC 9110, 15.2.1
        case http::StatusSwitchingProtocols : return "Switching Protocols"; // RFC 9110, 15.2.2
        case http::StatusProcessing         : return "Processing"; // RFC 2518, 10.1
        case http::StatusEarlyHints         : return "EarlyHints"; // RFC 8297

        // 200
        case http::StatusOK                   : return "OK"; // RFC 9110, 15.3.1
        case http::StatusCreated              : return "Created"; // RFC 9110, 15.3.2
        case http::StatusAccepted             : return "Accepted"; // RFC 9110, 15.3.3
        case http::StatusNonAuthoritativeInfo : return "Non Authoritative Info"; // RFC 9110, 15.3.4
        case http::StatusNoContent            : return "No Content"; // RFC 9110, 15.3.5
        case http::StatusResetContent         : return "Reset Content"; // RFC 9110, 15.3.6
        case http::StatusPartialContent       : return "Partial Content"; // RFC 9110, 15.3.7
        case http::StatusMultiStatus          : return "Multi Status"; // RFC 4918, 11.1
        case http::StatusAlreadyReported      : return "Already Reported"; // RFC 5842, 7.1
        case http::StatusIMUsed               : return "IM Used"; // RFC 3229, 10.4.1

        // 300
        case http::StatusMultipleChoices   : return "Multiple Choices"; // RFC 9110, 15.4.1
        case http::StatusMovedPermanently  : return "Moved Permanently"; // RFC 9110, 15.4.2
        case http::StatusFound             : return "Found"; // RFC 9110, 15.4.3
        case http::StatusSeeOther          : return "See Other"; // RFC 9110, 15.4.4
        case http::StatusNotModified       : return "Not Modified"; // RFC 9110, 15.4.5
        case http::StatusUseProxy          : return "Use Proxy"; // RFC 9110, 15.4.6
        case http::StatusTemporaryRedirect : return "Temporary Redirect"; // RFC 9110, 15.4.8
        case http::StatusPermanentRedirect : return "Permanent Redirect"; // RFC 9110, 15.4.9

        // 400
        case http::StatusBadRequest                   : return "Bad Request"; // RFC 9110, 15.5.1
        case http::StatusUnauthorized                 : return "Unauthorized"; // RFC 9110, 15.5.2
        case http::StatusPaymentRequired              : return "Payment Required"; // RFC 9110, 15.5.3
        case http::StatusForbidden                    : return "Forbidden"; // RFC 9110, 15.5.4
        case http::StatusNotFound                     : return "Not Found"; // RFC 9110, 15.5.5
        case http::StatusMethodNotAllowed             : return "Method Not Allowed"; // RFC 9110, 15.5.6
        case http::StatusNotAcceptable                : return "Not Acceptable"; // RFC 9110, 15.5.7
        case http::StatusProxyAuthRequired            : return "Proxy AuthRequired"; // RFC 9110, 15.5.8
        case http::StatusRequestTimeout               : return "Request Timeout"; // RFC 9110, 15.5.9
        case http::StatusConflict                     : return "Conflict"; // RFC 9110, 15.5.10
        case http::StatusGone                         : return "Gone"; // RFC 9110, 15.5.11
        case http::StatusLengthRequired               : return "Length Required"; // RFC 9110, 15.5.12
        case http::StatusPreconditionFailed           : return "Precondition Failed"; // RFC 9110, 15.5.13
        case http::StatusRequestEntityTooLarge        : return "Request Entity TooLarge"; // RFC 9110, 15.5.14
        case http::StatusRequestURITooLong            : return "Request URI TooLong"; // RFC 9110, 15.5.15
        case http::StatusUnsupportedMediaType         : return "Unsupported Media Type"; // RFC 9110, 15.5.16
        case http::StatusRequestedRangeNotSatisfiable : return "Requested Range Not Satisfiable"; // RFC 9110, 15.5.17
        case http::StatusExpectationFailed            : return "Expectation Failed"; // RFC 9110, 15.5.18
        case http::StatusTeapot                       : return "Teapot"; // RFC 9110, 15.5.19 (Unused)
        case http::StatusMisdirectedRequest           : return "Misdirected Request"; // RFC 9110, 15.5.20
        case http::StatusUnprocessableEntity          : return "Unprocessable Entity"; // RFC 9110, 15.5.21
        case http::StatusLocked                       : return "Locked"; // RFC 4918, 11.3
        case http::StatusFailedDependency             : return "Failed Dependency"; // RFC 4918, 11.4
        case http::StatusTooEarly                     : return "Too Early"; // RFC 8470, 5.2.
        case http::StatusUpgradeRequired              : return "Upgrade Required"; // RFC 9110, 15.5.22
        case http::StatusPreconditionRequired         : return "Precondition Required"; // RFC 6585, 3
        case http::StatusTooManyRequests              : return "Too Many Requests"; // RFC 6585, 4
        case http::StatusRequestHeaderFieldsTooLarge  : return "Request Header Fields TooLarge"; // RFC 6585, 5
        case http::StatusUnavailableForLegalReasons   : return "Unavailable For Legal Reasons"; // RFC 7725, 3

        // 500
        case http::StatusInternalServerError           : return "Internal Server Error"; // RFC 9110, 15.6.1
        case http::StatusNotImplemented                : return "Not Implemented"; // RFC 9110, 15.6.2
        case http::StatusBadGateway                    : return "Bad Gateway"; // RFC 9110, 15.6.3
        case http::StatusServiceUnavailable            : return "Service Unavailable"; // RFC 9110, 15.6.4
        case http::StatusGatewayTimeout                : return "Gateway Timeout"; // RFC 9110, 15.6.5
        case http::StatusHTTPVersionNotSupported       : return "HTTP Version Not Supported"; // RFC 9110, 15.6.6
        case http::StatusVariantAlsoNegotiates         : return "Variant Also Negotiates"; // RFC 2295, 8.1
        case http::StatusInsufficientStorage           : return "Insufficient Storage"; // RFC 4918, 11.5
        case http::StatusLoopDetected                  : return "Loop Detected"; // RFC 5842, 7.2
        case http::StatusNotExtended                   : return "Not Extended"; // RFC 2774, 7
        case http::StatusNetworkAuthenticationRequired : return "Network Authentication Required"; // RFC 6585, 6
        default: return "Unknown";
    }
}