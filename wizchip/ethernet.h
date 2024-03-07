#ifndef WIZCHIP_ETHERNET_H
#define WIZCHIP_ETHERNET_H

#include "Core/Inc/spi.h"
#include "periph/gpio.h"
#include "etl/function.h"
#include "wizchip_conf.h"

#ifndef WIZCHIP_BUFFER_LENGTH
#define WIZCHIP_BUFFER_LENGTH 2048
#endif

namespace Project::wizchip {
    class Ethernet;

    /// Represents a socket for network communication.
    class Socket {
        friend class Ethernet;
        
    public:
        struct Args {
            Ethernet& ethernet; ///< reference to the Ethernet instance
            int port;
        };
        
        /// default constructor
        constexpr explicit Socket(Args args) : ethernet(args.ethernet), port(args.port) {}

        /// disable copy constructor and assignment
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        int getPort() const { return port; }
        
    protected:
        virtual int on_init(int socket_number) = 0;
        virtual int on_listen(int socket_number) = 0;
        virtual int on_established(int socket_number) = 0;
        virtual int on_close_wait(int socket_number) = 0;
        virtual int on_closed(int socket_number) = 0;

        int allocate(int number_of_socket);
        void deallocate(int socket_number);

        Ethernet& ethernet;
        int port;
    };

    /// Represents an Ethernet interface for network communication.
    class Ethernet {
        friend class Socket;

    public:
        static uint8_t rxData[WIZCHIP_BUFFER_LENGTH];
        static uint8_t txData[WIZCHIP_BUFFER_LENGTH];

        /// Arguments structure for initializing the Ethernet class.
        struct Args {
            SPI_HandleTypeDef& hspi;                ///< SPI handler
            periph::GPIO cs;                        ///< Chip select pin.
            periph::GPIO rst;                       ///< Reset pin.
            wiz_NetInfo netInfo = DefaultNetInfo;   ///< network information.
        };

        /// default constructor
        constexpr explicit Ethernet(Args args) : hspi(args.hspi), cs(args.cs), rst(args.rst), netInfo(args.netInfo) {}

        /// disable copy constructor and assignment
        Ethernet(const Ethernet&) = delete;
        Ethernet& operator=(const Ethernet&) = delete;
        
        void init();
        void deinit();

        void setNetInfo(const wiz_NetInfo& netInfo);
        const wiz_NetInfo& getNetInfo();

        struct Debug {
            etl::Function<void(const char*), void*> fn;
            Debug& operator=(etl::Function<void(const char*), void*> fn) { this->fn = fn; return *this; }
            Debug& operator<<(const char* msg) { fn(msg); return *this; }
        } debug;

    private:
        void execute();

        SPI_HandleTypeDef& hspi;
        periph::GPIO cs;
        periph::GPIO rst;
        wiz_NetInfo netInfo;

        bool isRunning = false;
        Socket* sockets[_WIZCHIP_SOCK_NUM_] = {};
        static Ethernet* self;

        inline static constexpr wiz_NetInfo DefaultNetInfo = { 
            .mac = { 0x00, 0x08, 0xdc, 0xff, 0xee, 0xdd},
            .ip = { 192, 168, 0, 130 },
            .sn = { 255, 255, 255, 0 },
            .gw = { 192, 168, 0, 254 },
            //.dns = { 168, 126, 63, 1 },
            .dns = { 8, 8, 8, 8 },
            .dhcp = NETINFO_STATIC,
        };
    };
} 

#endif // WIZCHIP_ETHERNET_H