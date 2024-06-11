# Wizchip interface for STM32
Inspired by [cpp-httplib](https://github.com/yhirose/cpp-httplib)

## Requirements
* Arm GNU toolchain
* cmake minimum version 3.10
* STM32CubeMx generated code

## Dependencies
* [Embedded Template Library](https://github.com/aufam/etl/tree/FreeRTOS)
* [STM32 HAL Interface](https://github.com/aufam/stm32_hal_interface)

## How to use
* Clone this repo to your STM32 project folder. For example:
```bash
git clone https://github.com/aufam/stm32_wizchip.git <your_project_path>/Middlewares/Third_Party/stm32_wizchip
cd <your_project_path>/Middlewares/Third_Party/stm32_wizchip
```
* Or, if Git is configured, you can add this repo as a submodule:
```bash
git submodule add https://github.com/aufam/stm32_wizchip.git <your_project_path>/Middlewares/Third_Party/stm32_wizchip
cd <your_project_path>/Middlewares/Third_Party/stm32_wizchip
git submodule update --init --recursive
```
* Link the library to your project in your CMakeLists.txt file:
```cmake
add_subdirectory(Middlewares/Third_Party/stm32_wizchip)
target_link_libraries(${PROJECT_NAME}.elf wizchip)
```

## Example code
Server:
```c++
#include "wizchip/http/server.h"
#include "etl/json.h"

using namespace Project;
using namespace Project::wizchip;

auto ethernet = Ethernet ({
    .hspi=hspi1,
    .cs={.port=CS_GPIO_Port, .pin=CS_Pin},
    .rst={.port=RESET_GPIO_Port, .pin=RESET_Pin},
    .netInfo={ 
        .mac={0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
        .ip={192, 168, 1, 10},
        .sn={255, 255, 255, 0},
        .gw={192, 168, 1, 1},
        .dns={8, 8, 8, 8},
        .dhcp=NETINFO_STATIC,
    },
});

auto server = http::Server({
    .port=5000,
});

void http_server_example() {
    ethernet.init();

    // global headers
    server.global_headers["Server"] = []() -> std::string {
        return "stm32-wizchip/" WIZCHIP_VERSION;
    };

    // print hello
    server.Get("/hello", "text/plain", [](const http::Request&, http::Response& response) {
        response.body = "Hello world";
    });

    // display all routes
    server.Get("/routes", "application/json", [] (const http::Request&, http::Response& response) {
        response.body = "[";
        for (auto &router : server.routers) {
            response.body += std::string() + "{"
                "\"method\":\"" + router.method + "\","
                "\"path\":\"" + router.path + "\","
                "\"responseContentType\":\"" + router.content_type + "\""
            "},";
        }
        response.body.back() = ']';
    });

    // extract queries
    server.Get("/queries", "application/json", [] (const http::Request& request, http::Response& response) {
        response.body = "{\"path\":\"" + request.path.full_path + "\"}";
        for (auto &[key, value] : request.path.queries) {
            response.body.back() = ',';
            response.body += "\"" + key +"\":\"" + value + "\"}";
        }
    });

    // post json 
    server.Post("/json", "application/json", [] (const http::Request& request, http::Response& response) {
        auto json = etl::Json::parse(etl::string_view(request.body.c_str(), request.body.size()));
        auto json_err = json.error_message();

        if (json_err) {
            response.status = http::StatusBadRequest;
            response.body = std::string() + "{\"errorJson\":\"" + json_err.data() + "\",\"body\":" + request.body + "\"}";
        } else {
            response.body = request.body;
        }
    });

    server.start();
}
```

Client:
```c++
#include "wizchip/http/client.h"

using namespace Project;
using namespace Project::wizchip;

// create ethernet object
auto ethernet = Ethernet({
    .hspi=hspi1,
    .cs={.port=GPIOA, .pin=GPIO_PIN_4},
    .rst={.port=GPIOD, .pin=GPIO_PIN_2},
    .netInfo={ 
        .mac={0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
        .ip={192, 168, 1, 10},
        .sn={255, 255, 255, 0},
        .gw={192, 168, 1, 1},
        .dns={8, 8, 8, 8},
        .dhcp=NETINFO_STATIC,
    },
});

void http_client_example() {
    ethernet.init();
    
    auto client = http::Client({
        .host={192, 168, 1, 11},
        .port=8080,
    });

    // example get request
    http::Response response1 = client.Get("/api").await().unwrap();

    // example post request with body
    http::Response response2 = client.Post("/api/2/valid", "Content-Type: text/plain", "Hello").await().unwrap();
}
```
