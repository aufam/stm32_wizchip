# STM32 Wizchip
Wizchip interface for STM32

## Requirements
* Arm GNU toolchain
* cmake minimum version 3.10
* STM32CubeMx generated code with:
    - FreeRTOS v2
    - SPI

## Dependencies
* [Embedded Template Library](https://github.com/aufam/etl/tree/FreeRTOS)
* [STM32 HAL Interface](https://github.com/aufam/stm32_hal_interface)

## How to use
* Clone this repo to your STM32 project folder. For example:
```bash
git clone https://github.com/aufam/stm32_wizchip.git <your_project_path>/Middlewares/Third_Party/stm32_wizchip
```
* Or, if Git is configured, you can add this repo as a submodule:
```bash
git submodule add https://github.com/aufam/stm32_wizchip.git <your_project_path>/Middlewares/Third_Party/stm32_wizchip
git submodule update --init --recursive
```
* Link the library to your project in your CMakeLists.txt file:
```cmake
add_subdirectory(Middlewares/Third_Party/stm32_wizchip)
target_link_libraries(${PROJECT_NAME}.elf wizchip)
```

## Setup Code
```c++
#include "wizchip/ethernet.h"

using namespace Project;
using namespace Project::wizchip;

auto ethernet = Ethernet ({
    .hspi=hspi1,
    .cs={.port=CS_GPIO_Port, .pin=CS_Pin},
    .rst={.port=RESET_GPIO_Port, .pin=RESET_Pin},
    .netInfo={ 
        .mac={0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
        .ip={10, 20, 30, 2},
        .sn={255, 255, 255, 0},
        .gw={10, 20, 30, 1},
        .dns={10, 20, 30, 1},
        .dhcp=NETINFO_STATIC,
    },
});

void setup_ethernet() {
    ethernet.init();
}
```

## Example HTTP Server
```c++
#include "wizchip/http/server.h"

using namespace Project;
using namespace Project::wizchip::http;

Server app;

// example custom struct
struct Foo {
    int num;
    std::string text;
};

JSON_DEFINE(Foo, 
    JSON_ITEM("num", num), 
    JSON_ITEM("text", text)
)

void setup_http_server() {
    // example: set additional global headers
    app.global_headers["Server"] = [](const Request&, const Response&) { 
        return "stm32-wizchip/" WIZCHIP_VERSION; 
    };

    // example: print hello
    app.Get("/hello", {}, 
    []() -> const char* {
        return "Hello world from stm32-wizchip/" WIZCHIP_VERSION;
    });

    // example: - get request param (in this case the body as string_view), - possible error return value
    app.Post("/body", std::tuple{arg::body},
    [](std::string_view body) -> etl::Result<std::string_view, Server::Error> {
        if (body.empty()) {
            return etl::Err(Server::Error{StatusBadRequest, "Body is empty"});
        } else {
            return etl::Ok(body);
        }
    });

    static const char* const access_token = "1234";
    
    auto get_token = [](const Request& req) -> etl::Result<std::string_view, Server::Error> {
        std::string_view token = "";
        if (req.headers.has("Authentication")) {
            token = req.headers["Authentication"];
        } else if (req.headers.has("authentication")) {
            token = req.headers["authentication"];
        } else {
            return etl::Err(Server::Error{StatusUnauthorized, "No auth provided"});
        }
        if (token == std::string("Bearer ") + access_token) {
            return etl::Ok(token);
        } else {
            return etl::Err(Server::Error{StatusUnauthorized, "Token doesn't match"});
        }
    };

    // example: 
    // - adding dependency (in this case is authentication token), 
    // - arg with default value, it will try to find "add" key in the request headers and request queries. 
    //   If not found, use the default value
    // - json deserialize request body to Foo
    app.Post("/foo", std::tuple{arg::depends(get_token), arg::default_val("add", 20), arg::json},
    [](std::string_view token, int add, Foo foo) -> Foo {
        return {foo.num + add, foo.text + ": " + std::string(token)}; 
    });

    // example:
    // multiple methods handler
    app.route("/methods", {"GET", "POST"}, std::tuple{arg::method},
    [](std::string_view method) {
        if (method == "GET") {
            return "Example GET method";
        } else {
            return "Example POST method";
        }
    });

    // example: print all headers
    app.Get("/headers", std::tuple{arg::headers},
    [](etl::Ref<const etl::UnorderedMap<std::string, std::string>> headers) {
        return headers;
    });

    // example: print all queries
    app.Get("/queries", std::tuple{arg::queries},
    [](etl::Ref<const etl::UnorderedMap<std::string, std::string>> queries) {
        return queries;
    });

    app.start({.port=5000, .number_of_socket=4});
}
```
