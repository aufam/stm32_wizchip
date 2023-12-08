# Wizchip interface for STM32

## Requirements
* C++17
* cmake minimum version 3.10
* STM32CubeMx generated code

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
```
* Link the library to your project in your CMakeLists.txt file:
```cmake
add_subdirectory(Middlewares/Third_Party/stm32_wizchip)
target_link_libraries(${PROJECT_NAME}.elf wizchip)
```

## Example code
```c++
#include "wizchip/http/server.h"
#include "etl/string.h"
#include "periph/usb.h"

using namespace Project;

// create ethernet object.
auto ethernet = wizchip::Ethernet({
    .hspi=hspi1,
    .cs={
        .port=GPIOA, 
        .pin=GPIO_PIN_4
    },
    .rst={
        .port=GPIOD, 
        .pin=GPIO_PIN_2
    },
    .netInfo={ 
        .mac = { 0x00, 0x08, 0xdc, 0xff, 0xee, 0xdd},
        .ip = { 192, 168, 0, 130 },
        .sn = { 255, 255, 255, 0 },
        .gw = { 192, 168, 0, 254 },
        .dns = { 8, 8, 8, 8 },
        .dhcp = NETINFO_STATIC,
    },
});

// create server object. mark as static
auto server = wizchip::http::Server({
    .ethernet=ethernet,
    .port=8080,
});

// string formatter 64 bytes for debug message
auto static f = etl::string<64>();

void http_server_example() {
    ethernet.debug = [] (const char* str) { periph::usb << str; }; // set debug print to usb
    ethernet.init();

    // example: create GET method
    server.Get("/api", [] (const wizchip::http::Request& request, wizchip::http::Response& response) {
        response.set_head("Content-Type", "text/plain");
        response.body = "Hello world!";

        ethernet.debug(f("%.*s %.*s %d\r\n", 
            request.method.len(), request.method.data(),
            request.url.len(), request.url.data(),
            response.status
        ));
    });

    // example: create POST method with args
    server.Post("/api/%s/%s", [] (const wizchip::http::Request& request, wizchip::http::Response& response) {
        // extract args
        int arg1 = request.matches(0).to_int();
        etl::StringView arg2 = request.matches(1);

        // assuming response head won't reach half of the tx buffer
        char* body = reinterpret_cast<char*>(wizchip::Ethernet::txData + (WIZCHIP_BUFFER_LENGTH / 2));
        sprintf(body, "{ \"arg1\": %d, \"arg2\": %.*s }", arg1, arg2.len(), arg2.data());

        size_t strlen_body = strlen(body);
        char* body_length = body + strlen_body + 1;
        sprintf(body_length, "%d", strlen_body);

        response.set_head("Content-Type", "application/json");
        response.set_head("Content-Length", body_length);
        response.body = body;

        // example set status
        if (arg1 == 2 or arg2 == "valid") {
            response.status = 200;
        } else {
            response.status = 205;
            response.status_string = "Bad request";
        }
        
        ethernet.debug(f("%.*s %.*s %d\r\n", 
            request.method.len(), request.method.data(),
            request.url.len(), request.url.data(),
            response.status
        ));
    });

    server.start();
}
```
