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
git submodule update --init --recursive
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
#include "etl/string.h"
#include "periph/usb.h"

using namespace Project;

// create ethernet object
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
        .mac={0x00, 0x08, 0xdc, 0xff, 0xee, 0xdd},
        .ip={192, 168, 0, 130},
        .sn={255, 255, 255, 0},
        .gw={192, 168, 0, 254},
        .dns={8, 8, 8, 8},
        .dhcp=NETINFO_STATIC,
    },
});

// create server object
auto server = wizchip::http::Server({
    .ethernet=ethernet,
    .port=8080,
});

// string formatter 64 bytes
auto static f = etl::string<64>();

void http_server_example() {
    ethernet.debug = [] (const char* str) { periph::usb << str; }; // set ethernet debug to usb
    ethernet.init();

    // example: create GET method
    server.Get("/api", [] (const wizchip::http::Request&, wizchip::http::Response& response) {
        response.head = "Content-Type: text/plain";
        response.body = "Hello world!";
    });

    // example: create POST method with args
    server.Post("/api/%s/%s", [] (const wizchip::http::Request& request, wizchip::http::Response& response) {
        // extract args
        int arg1 = request.matches(0).to_int();
        etl::StringView arg2 = request.matches(1);

        // assuming response head won't reach half of the tx buffer
        char* body = reinterpret_cast<char*>(wizchip::Ethernet::txData + (WIZCHIP_BUFFER_LENGTH / 2));
        sprintf(body, 
            "{ \"arg1\": %d, \"arg2\": %.*s, \"body\": %.*s }", 
            arg1, arg2.len(), arg2.data(), request.body.len(), request.body.data()
        );
        response.head = f("Content-Type: text/plain" "\r\n" "Content-Length: %d", strlen(body));
        response.body = body;

        // example set status
        if (arg1 == 2 or arg2 == "valid") {
            response.status = 200;
        } else {
            response.status = 205;
            response.status_string = "Bad request";
        }
    });

    server.debug = [] (const wizchip::http::Request& request, const wizchip::http::Response& response) {
        periph::usb << f("%.*s %.*s %d\r\n", 
            request.method.len(), request.method.data(),
            request.url.len(), request.url.data(),
            response.status
        );
    }; // set server debug to usb

    server.start();
}
```

Client:
```c++

#include "wizchip/http/client.h"
#include "periph/usb.h"

using namespace Project;

// create ethernet object
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
        .mac={0x00, 0x08, 0xdc, 0xff, 0xee, 0xdd},
        .ip={192, 168, 0, 131},
        .sn={255, 255, 255, 0},
        .gw={192, 168, 0, 254},
        .dns={8, 8, 8, 8},
        .dhcp=NETINFO_STATIC,
    },
});

void http_client_example() {
    ethernet.debug = [] (const char* str) { periph::usb << str; }; // set ethernet debug to usb
    ethernet.init();
    
    auto client = wizchip::http::Client({
        .ethernet=ethernet,
        .host="192.168.0.130",
        .port=8080,
    });

    wizchip::http::Response response1 = client.Get("/api");
    // process(response1);

    wizchip::http::Response response2 = client.Post("/api/2/valid", "Content-Type: text/plain", "Hello");
    // process(response2);
}
```
