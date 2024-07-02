#ifndef WIZCHIP_ETHERNET_H
#define WIZCHIP_ETHERNET_H

#include "wizchip_conf.h"
#include "Core/Inc/spi.h"
#include "periph/gpio.h"
#include "etl/mutex.h"
#include "etl/vector.h"
#include "etl/future.h"

namespace Project::wizchip {
    class SocketServer;
    class SocketSession;

    /// Represents an Ethernet interface for network communication.
    [[singleton]]
    class Ethernet {
        friend class SocketServer;
        friend class SocketSession;

    public:
        /// Arguments structure for initializing the Ethernet class.
        struct Args {
            SPI_HandleTypeDef& hspi;                ///< SPI handler
            periph::GPIO cs;                        ///< Chip select pin.
            periph::GPIO rst;                       ///< Reset pin.
            wiz_NetInfo netInfo;                    ///< network information.
        };

        /// default constructor
        explicit Ethernet(Args args) : hspi(args.hspi), cs(args.cs), rst(args.rst), netInfo(args.netInfo) {}
        
        static Ethernet* self;

        /// disable copy constructor and assignment
        Ethernet(const Ethernet&) = delete;
        Ethernet& operator=(const Ethernet&) = delete;
        
        void init();
        void deinit();
        bool isRunning() const;

        void setNetInfo(const wiz_NetInfo& netInfo);
        const wiz_NetInfo& getNetInfo();

        struct Logger {
            std::function<void(const char*)> function;
            Logger& operator<<(const char* msg) { if (function) function(msg); return *this; }
        } logger = {};

        etl::Mutex mutex;

    private:
        void execute();

        SPI_HandleTypeDef& hspi;
        periph::GPIO cs;
        periph::GPIO rst;
        wiz_NetInfo netInfo;

        bool _is_running = false;

        struct SocketHandler {
            SocketServer* socket_interface;    
            bool socket_session;
            bool is_busy() const { return socket_interface || socket_session; }
        };
        SocketHandler socket_handlers[_WIZCHIP_SOCK_NUM_] = {};
    };

    /// Represents a socket for network communication.
    class SocketServer {
        friend class Ethernet;
        
    public:
        constexpr SocketServer() : port(0) {}

        /// disable copy constructor and assignment
        SocketServer(const SocketServer&) = delete;
        SocketServer& operator=(const SocketServer&) = delete;

        struct StartArgs {
            int port;
            int number_of_socket = 1;
        };

        void start(StartArgs);
        void stop();
        bool isRunning() const;

        int port;

    protected:
        virtual const char* kind() = 0;
        virtual int on_init(int socket_number) = 0;
        virtual int on_listen(int socket_number) = 0;
        virtual int on_established(int socket_number) = 0;
        virtual int on_close_wait(int socket_number) = 0;
        virtual int on_closed(int socket_number) = 0;
        virtual etl::Vector<uint8_t> response(etl::Vector<uint8_t>) = 0;

        etl::Vector<int> reserved_sockets;
    };

    class SocketSession {
        friend class Ethernet;
    
    public:
        SocketSession(uint8_t protocol, uint8_t flag, etl::Vector<uint8_t> host, int port);
        ~SocketSession();

        etl::Vector<uint8_t> host;
        int port;
        int socket_number;
    };
} 

namespace Project::wizchip::detail {
    etl::Vector<uint8_t> ipv4_to_bytes(const char* ip);
    etl::Pair<etl::Vector<uint8_t>, uint16_t> ipv4_port_to_pair(const char* ip_port);
    
    etl::Result<void, osStatus_t> tcp_transmit(int socket_number, etl::Vector<uint8_t> data);
    etl::Result<etl::Vector<uint8_t>, osStatus_t> tcp_receive(int socket_number);

    etl::Result<void, osStatus_t> udp_transmit(int socket_number, etl::Vector<uint8_t> ip, int port, etl::Vector<uint8_t> data);
    etl::Result<etl::Vector<uint8_t>, osStatus_t> udp_receive(int socket_number, etl::Vector<uint8_t> ip);
}

#endif // WIZCHIP_ETHERNET_H